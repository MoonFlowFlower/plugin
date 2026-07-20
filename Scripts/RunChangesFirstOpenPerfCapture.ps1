[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [int]$EditorLaunchTimeoutSeconds = 180,
    [int]$BridgeTimeoutSeconds = 60
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$BridgeStatePath = Join-Path $ProjectRoot "Saved\UE_MCP_Bridge\bridge_state.json"
$ValidationRoot = Join-Path $ProjectRoot "Saved\RuntimeInspector\Validation"
$CaptureTestId = "capture_changes_first_open_perf"

function Test-TcpPort {
    param(
        [string]$HostName = "127.0.0.1",
        [int]$Port,
        [int]$TimeoutMs = 250
    )

    $Client = New-Object System.Net.Sockets.TcpClient
    try {
        $Async = $Client.BeginConnect($HostName, $Port, $null, $null)
        if (-not $Async.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) {
            return $false
        }
        $Client.EndConnect($Async) | Out-Null
        return $true
    } catch {
        return $false
    } finally {
        $Client.Dispose()
    }
}

function Get-BridgeState {
    if (-not (Test-Path $BridgeStatePath -PathType Leaf)) {
        return $null
    }

    try {
        return Get-Content -LiteralPath $BridgeStatePath -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Wait-EditorBridge {
    param([int]$TimeoutSeconds)

    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $Deadline) {
        $State = Get-BridgeState
        if ($State -and $State.active -and $State.port -and (Test-TcpPort -Port ([int]$State.port))) {
            return $State
        }
        Start-Sleep -Seconds 1
    }

    throw "Editor bridge did not become ready within $TimeoutSeconds seconds."
}

function Invoke-BridgeRequest {
    param(
        [object]$BridgeState,
        [string]$Method,
        [hashtable]$Params = @{},
        [int]$TimeoutSeconds = 30
    )

    $WebSocket = [System.Net.WebSockets.ClientWebSocket]::new()
    $Cancellation = [System.Threading.CancellationTokenSource]::new()
    $Cancellation.CancelAfter($TimeoutSeconds * 1000)
    $Uri = [Uri]::new(("ws://127.0.0.1:{0}" -f [int]$BridgeState.port))
    $WebSocket.ConnectAsync($Uri, $Cancellation.Token).GetAwaiter().GetResult()

    try {
        $Payload = @{
            jsonrpc = "2.0"
            id = [Guid]::NewGuid().ToString("N")
            method = $Method
            params = $Params
        } | ConvertTo-Json -Depth 12 -Compress

        $PayloadBytes = [System.Text.Encoding]::UTF8.GetBytes($Payload)
        $Segment = [System.ArraySegment[byte]]::new($PayloadBytes)
        $WebSocket.SendAsync($Segment, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, $Cancellation.Token).GetAwaiter().GetResult()

        $Buffer = New-Object byte[] 65536
        $Builder = New-Object System.Text.StringBuilder
        do {
            $ReceiveSegment = [System.ArraySegment[byte]]::new($Buffer)
            $ReceiveResult = $WebSocket.ReceiveAsync($ReceiveSegment, $Cancellation.Token).GetAwaiter().GetResult()
            if ($ReceiveResult.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) {
                throw "Bridge closed the websocket before returning a result."
            }
            if ($ReceiveResult.Count -gt 0) {
                [void]$Builder.Append([System.Text.Encoding]::UTF8.GetString($Buffer, 0, $ReceiveResult.Count))
            }
        } while (-not $ReceiveResult.EndOfMessage)

        $Response = $Builder.ToString() | ConvertFrom-Json
        if ($Response.error) {
            throw ($Response.error.message | Out-String).Trim()
        }

        return $Response.result
    } finally {
        try {
            if ($WebSocket.State -eq [System.Net.WebSockets.WebSocketState]::Open) {
                $WebSocket.CloseOutputAsync([System.Net.WebSockets.WebSocketCloseStatus]::NormalClosure, "done", [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()
            }
        } catch {
        }
        $WebSocket.Dispose()
        $Cancellation.Dispose()
    }
}

function Ensure-EditorReady {
    $State = Get-BridgeState
    if ($State -and $State.active -and $State.port -and (Test-TcpPort -Port ([int]$State.port))) {
        return $State
    }

    if (-not (Test-Path $EditorExe -PathType Leaf)) {
        throw "UnrealEditor.exe not found at: $EditorExe"
    }
    if (-not (Test-Path $ProjectFile -PathType Leaf)) {
        throw "Project file not found at: $ProjectFile"
    }

    Start-Process -FilePath $EditorExe -ArgumentList @($ProjectFile, "-NoSplash") | Out-Null
    return Wait-EditorBridge -TimeoutSeconds $EditorLaunchTimeoutSeconds
}

function Wait-ForFile {
    param(
        [string]$Path,
        [int]$TimeoutSeconds = 15
    )

    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath $Path -PathType Leaf) {
            return $true
        }
        Start-Sleep -Milliseconds 250
    }

    return $false
}

function Ensure-PieRunning {
    param(
        [object]$BridgeState,
        [int]$TimeoutSeconds = 60
    )

    $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds $TimeoutSeconds
    if (-not $PieStatus.success) {
        throw "Failed to query PIE status."
    }

    if ($PieStatus.isPlaying) {
        [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "stop" } -TimeoutSeconds 120)
        $StopDeadline = (Get-Date).AddSeconds($TimeoutSeconds)
        do {
            Start-Sleep -Seconds 1
            $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds $TimeoutSeconds
            if (-not $PieStatus.isPlaying) {
                break
            }
        } while ((Get-Date) -lt $StopDeadline)
    }

    [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "start" } -TimeoutSeconds 120)
    $StartDeadline = (Get-Date).AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Seconds 1
        $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds $TimeoutSeconds
        if ($PieStatus.isPlaying) {
            return
        }
    } while ((Get-Date) -lt $StartDeadline)

    throw "Failed to start PIE before validation capture."
}

function Resolve-ReportPathFromText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return $null
    }

    $Match = [regex]::Match($Text, "Report=([^\r\n]+report\.json)")
    if ($Match.Success) {
        return $Match.Groups[1].Value.Trim()
    }

    return $null
}

function Get-WindowTitleFromSummary {
    param([string]$Summary)

    if ([string]::IsNullOrWhiteSpace($Summary)) {
        return $null
    }

    if ($Summary.StartsWith("Title=")) {
        $WithoutPrefix = $Summary.Substring(6)
        $Parts = $WithoutPrefix -split " \| ", 2
        if ($Parts.Count -gt 0 -and -not [string]::IsNullOrWhiteSpace($Parts[0])) {
            return $Parts[0].Trim()
        }
    }

    $LooseMatch = [regex]::Match($Summary, "Title=(.*?)( \\| |$)")
    if ($LooseMatch.Success) {
        return $LooseMatch.Groups[1].Value.Trim()
    }

    return $null
}

New-Item -ItemType Directory -Force -Path $ValidationRoot | Out-Null

$BridgeState = Ensure-EditorReady
Ensure-PieRunning -BridgeState $BridgeState -TimeoutSeconds $BridgeTimeoutSeconds

$SelfTestCatalog = Invoke-BridgeRequest -BridgeState $BridgeState -Method "list_runtime_self_tests" -TimeoutSeconds 60
$KnownTest = $SelfTestCatalog.selfTests | Where-Object { $_.id -eq $CaptureTestId }
if (-not $KnownTest) {
    throw "Unknown RuntimeInspector self test id: $CaptureTestId"
}

$SelfTestResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_self_test" -Params @{ testId = $CaptureTestId } -TimeoutSeconds 180
$ReportPath = Resolve-ReportPathFromText -Text $SelfTestResult.fullReport
$CaptureReport = $null
$CaptureDir = $ValidationRoot
$CaptureId = $null
if ($ReportPath -and (Test-Path -LiteralPath $ReportPath -PathType Leaf)) {
    $CaptureReport = Get-Content -LiteralPath $ReportPath -Raw | ConvertFrom-Json
    $CaptureDir = Split-Path -Parent $ReportPath
    $CaptureId = [string]$CaptureReport.captureId
}

New-Item -ItemType Directory -Force -Path $CaptureDir | Out-Null
$BridgeScreenshotPath = Join-Path $CaptureDir "changes-first-open-bridge.png"
Remove-Item -LiteralPath $BridgeScreenshotPath -Force -ErrorAction SilentlyContinue

[void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "open" } -TimeoutSeconds 30)
[void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "show_page"; page = "Changes" } -TimeoutSeconds 30)
Start-Sleep -Milliseconds 750
$AutomationSummary = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary" -TimeoutSeconds 30
$HostWindowTitle = Get-WindowTitleFromSummary -Summary ([string]$AutomationSummary.panelHostWindowDebug)
$CaptureParams = @{
    filename = $BridgeScreenshotPath
    showUI = $true
}
if (-not [string]::IsNullOrWhiteSpace($HostWindowTitle)) {
    $CaptureParams.windowTitleContains = $HostWindowTitle
}
$CaptureResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "capture_window_screenshot" -Params $CaptureParams -TimeoutSeconds 60
$BridgeScreenshotReady = Wait-ForFile -Path $BridgeScreenshotPath -TimeoutSeconds 15

$KeyLogs = @()
if ($CaptureReport -and $CaptureReport.keyLogLines) {
    foreach ($Line in $CaptureReport.keyLogLines) {
        $KeyLogs += [string]$Line
    }
}
if ($BridgeScreenshotReady) {
    $KeyLogs += "[RI][Capture] ExternalBridgeScreenshot Path=$BridgeScreenshotPath"
}

$Metrics = @{}
if ($CaptureReport -and $CaptureReport.metrics) {
    foreach ($Metric in $CaptureReport.metrics) {
        $Metrics[[string]$Metric.name] = [double]$Metric.valueMs
    }
}

$Verdict = "blocked"
if ($SelfTestResult.success -and $CaptureReport) {
    if ($BridgeScreenshotReady -and -not [string]::IsNullOrWhiteSpace($HostWindowTitle) -and [string]$CaptureResult.captureMode -eq "window" -and $Metrics.ContainsKey("ShowFilePage") -and $Metrics.ContainsKey("FileFastRefresh") -and $Metrics.ContainsKey("SharedContextStrip")) {
        $Verdict = "pass"
    } elseif ($Metrics.Count -gt 0) {
        $Verdict = "partial"
    } else {
        $Verdict = "fail"
    }
}

$CombinedArtifactPath = Join-Path $CaptureDir "external_runner.json"
$CombinedArtifact = [ordered]@{
    scenarioId = "changes_first_open_perf_capture"
    captureId = $CaptureId
    verdict = $Verdict
    bridgePort = [int]$BridgeState.port
    pluginCaptureReportPath = $ReportPath
    pluginCapturePassed = if ($CaptureReport) { [bool]$CaptureReport.bPassed } else { $false }
    pluginCaptureSummary = [string]$SelfTestResult.summary
    externalScreenshotPath = $BridgeScreenshotPath
    externalScreenshotReady = [bool]$BridgeScreenshotReady
    screenshotCaptureSuccess = [bool]$CaptureResult.success
    hostWindowTitle = [string]$HostWindowTitle
    captureMode = [string]$CaptureResult.captureMode
    captureWindowTitle = [string]$CaptureResult.windowTitle
    panelHostWindowDebug = [string]$AutomationSummary.panelHostWindowDebug
    pageState = if ($CaptureReport) { $CaptureReport.pageState } else { $null }
    selectedActor = if ($CaptureReport) { $CaptureReport.selectedActor } else { $null }
    sessionState = if ($CaptureReport) { $CaptureReport.sessionState } else { $null }
    metrics = $Metrics
    keyLogLines = $KeyLogs
}

$CombinedArtifact | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $CombinedArtifactPath -Encoding UTF8
$CombinedArtifact | ConvertTo-Json -Depth 12

if ($Verdict -eq "pass") {
    exit 0
}
if ($Verdict -eq "partial") {
    exit 2
}
exit 3
