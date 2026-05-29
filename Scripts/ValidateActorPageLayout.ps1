[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Software\Unreal\UE_5.5",
    [string]$TestId = "actor_page_structure",
    [string]$ScreenshotName = "actor_page_structure.png",
    [int]$EditorLaunchTimeoutSeconds = 180
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$BridgeStatePath = Join-Path $ProjectRoot "Saved\UE_MCP_Bridge\bridge_state.json"
$OutputDir = Join-Path $ProjectRoot "Saved\RuntimeInspector\Validation"
$ScreenshotPath = Join-Path $OutputDir $ScreenshotName

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

        $Buffer = New-Object byte[] 32768
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

function Get-WindowTitleFromSummary {
    param([string]$Summary)

    if ([string]::IsNullOrWhiteSpace($Summary)) {
        return $null
    }

    if ($Summary.StartsWith("Title=")) {
        $WithoutPrefix = $Summary.Substring(6)
        $Parts = $WithoutPrefix -split " \| Size=", 2
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

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Remove-Item -LiteralPath $ScreenshotPath -Force -ErrorAction SilentlyContinue

$BridgeState = Ensure-EditorReady
$PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds 90
if (-not $PieStatus.success) {
    throw "Failed to query PIE status."
}

if ($PieStatus.isPlaying) {
    [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "stop" } -TimeoutSeconds 120)
    $PieStopDeadline = (Get-Date).AddSeconds(60)
    do {
        Start-Sleep -Seconds 1
        $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds 90
        if (-not $PieStatus.isPlaying) {
            break
        }
    } while ((Get-Date) -lt $PieStopDeadline)
    if ($PieStatus.isPlaying) {
        throw "Failed to stop PIE before fresh actor page validation."
    }
}

[void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "start" } -TimeoutSeconds 120)
$PieDeadline = (Get-Date).AddSeconds(60)
do {
    Start-Sleep -Seconds 1
    $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds 90
    if ($PieStatus.isPlaying) {
        break
    }
} while ((Get-Date) -lt $PieDeadline)
if (-not $PieStatus.isPlaying) {
    throw "Failed to start PIE before actor page validation."
}

$SelfTestCatalog = Invoke-BridgeRequest -BridgeState $BridgeState -Method "list_runtime_self_tests" -TimeoutSeconds 60
$KnownTest = $SelfTestCatalog.selfTests | Where-Object { $_.id -eq $TestId }
if (-not $KnownTest) {
    throw "Unknown RuntimeInspector self test id: $TestId"
}

Start-Sleep -Seconds 2
$Attempt = 1
$ToggleResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "toggle_input" } -TimeoutSeconds 30
Start-Sleep -Milliseconds 750
$AutomationSummary = $null
for ($SummaryAttempt = 1; $SummaryAttempt -le 8; $SummaryAttempt++) {
    $AutomationSummary = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary" -TimeoutSeconds 30
    if ($AutomationSummary.panelHostWindowDebug -and $AutomationSummary.panelHostWindowDebug -notlike "PanelWidget=NoCachedWidget*") {
        break
    }
    Start-Sleep -Milliseconds 350
}
$WindowTitle = Get-WindowTitleFromSummary -Summary $AutomationSummary.panelHostWindowDebug
$WindowTitleFallbackUsed = $false
if ([string]::IsNullOrWhiteSpace($WindowTitle)) {
    $WindowTitle = "PluginMaker Preview"
    $WindowTitleFallbackUsed = $true
}

$CaptureResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "capture_window_screenshot" -Params @{
    filename = $ScreenshotPath
    showUI = $true
    windowTitleContains = $WindowTitle
} -TimeoutSeconds 60

$ScreenshotReady = Wait-ForFile -Path $ScreenshotPath -TimeoutSeconds 15

$SelfTestResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_self_test" -Params @{ testId = $TestId } -TimeoutSeconds 120

$Result = [ordered]@{
    bridgePort = [int]$BridgeState.port
    testId = $TestId
    toggleSuccess = [bool]$ToggleResult.success
    toggleOpen = [bool]$ToggleResult.isOpen
    toggleHandled = [bool]$ToggleResult.inputHandled
    toggleKey = [string]$ToggleResult.toggleKey
    toggleAttempts = [int]$Attempt
    summaryAttempts = [int]$SummaryAttempt
    selfTestSuccess = [bool]$SelfTestResult.success
    selfTestPassed = [bool]$SelfTestResult.passed
    selfTestSummary = [string]$SelfTestResult.summary
    selfTestReport = [string]$SelfTestResult.fullReport
    propertyHostDebug = [string]$AutomationSummary.propertyHostDebug
    propertyAnchorChain = [string]$AutomationSummary.propertyAnchorChain
    inspectBodyChildren = [string]$AutomationSummary.inspectBodyChildren
    panelPresentationDebug = [string]$AutomationSummary.panelPresentationDebug
    pageRoutingDebug = [string]$AutomationSummary.pageRoutingDebug
    actorTopContextValues = [string]$AutomationSummary.actorTopContextValues
    actorFooterDebug = [string]$AutomationSummary.actorFooterDebug
    contextLabelChains = [string]$AutomationSummary.contextLabelChains
    panelHostWindowDebug = [string]$AutomationSummary.panelHostWindowDebug
    windowTitle = [string]$WindowTitle
    windowTitleFallbackUsed = [bool]$WindowTitleFallbackUsed
    hostWindowTitle = [string]$WindowTitle
    captureSuccess = [bool]$CaptureResult.success
    captureMode = [string]$CaptureResult.captureMode
    captureWindowTitle = [string]$CaptureResult.windowTitle
    screenshotPath = [string]$ScreenshotPath
    screenshotReady = [bool]$ScreenshotReady
}

$Result | ConvertTo-Json -Depth 8

if (-not $SelfTestResult.passed) {
    exit 2
}

if (-not $ScreenshotReady -or [string]::IsNullOrWhiteSpace($WindowTitle) -or [string]$CaptureResult.captureMode -ne "window") {
    exit 3
}
