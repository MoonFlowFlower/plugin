[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [int]$EditorLaunchTimeoutSeconds = 180,
    [int]$BridgeTimeoutSeconds = 90
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$BridgeStatePath = Join-Path $ProjectRoot "Saved\UE_MCP_Bridge\bridge_state.json"
$ValidationRoot = Join-Path $ProjectRoot "Saved\RuntimeInspector\Validation\TransformSourcePersistence"
$PrepareTestId = "transform_source_persistence_prepare"
$VerifyTestId = "transform_source_persistence_verify_restore"

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
        } | ConvertTo-Json -Depth 16 -Compress

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

function Wait-ForPieState {
    param(
        [object]$BridgeState,
        [bool]$ExpectedPlaying,
        [int]$TimeoutSeconds = 60
    )

    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $Deadline) {
        Start-Sleep -Seconds 1
        $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds $TimeoutSeconds
        if ([bool]$PieStatus.isPlaying -eq $ExpectedPlaying) {
            return $PieStatus
        }
    }

    $ExpectedLabel = if ($ExpectedPlaying) { "playing" } else { "stopped" }
    throw ("Timed out waiting for PIE state {0}" -f $ExpectedLabel)
}

function Restart-PieFresh {
    param(
        [object]$BridgeState,
        [int]$TimeoutSeconds = 90
    )

    $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds $TimeoutSeconds
    if (-not $PieStatus.success) {
        throw "Failed to query PIE status."
    }

    if ($PieStatus.isPlaying) {
        [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "stop" } -TimeoutSeconds 120)
        [void](Wait-ForPieState -BridgeState $BridgeState -ExpectedPlaying:$false -TimeoutSeconds $TimeoutSeconds)
    }

    [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "start" } -TimeoutSeconds 120)
    [void](Wait-ForPieState -BridgeState $BridgeState -ExpectedPlaying:$true -TimeoutSeconds $TimeoutSeconds)
}

function Resolve-ReportPathFromText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return $null
    }

    $Match = [regex]::Match($Text, "Report=([^\s\r\n]+?\.json)(?:\s|$)")
    if ($Match.Success) {
        return $Match.Groups[1].Value.Trim()
    }

    return $null
}

function Resolve-PendingPathFromText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return $null
    }

    $Match = [regex]::Match($Text, "PendingState=([^\s\r\n]+pending_state\.json)(?:\s|$)")
    if ($Match.Success) {
        return $Match.Groups[1].Value.Trim()
    }

    return $null
}

New-Item -ItemType Directory -Force -Path $ValidationRoot | Out-Null

$BridgeState = Ensure-EditorReady

$SelfTestCatalog = Invoke-BridgeRequest -BridgeState $BridgeState -Method "list_runtime_self_tests" -TimeoutSeconds 60
foreach ($RequiredId in @($PrepareTestId, $VerifyTestId)) {
    if (-not ($SelfTestCatalog.selfTests | Where-Object { $_.id -eq $RequiredId })) {
        throw "Unknown RuntimeInspector self test id: $RequiredId"
    }
}

Restart-PieFresh -BridgeState $BridgeState -TimeoutSeconds $BridgeTimeoutSeconds
Start-Sleep -Seconds 2

$PrepareResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_self_test" -Params @{ testId = $PrepareTestId } -TimeoutSeconds 240
$PrepareReportPath = Resolve-ReportPathFromText -Text $PrepareResult.fullReport
$PendingPath = Resolve-PendingPathFromText -Text $PrepareResult.fullReport
$PrepareReport = $null
$CaptureDir = $ValidationRoot
$CaptureId = $null
if ($PrepareReportPath -and (Test-Path -LiteralPath $PrepareReportPath -PathType Leaf)) {
    $PrepareReport = Get-Content -LiteralPath $PrepareReportPath -Raw | ConvertFrom-Json
    $CaptureDir = Split-Path -Parent $PrepareReportPath
    $CaptureId = [string]$PrepareReport.captureId
}

if (-not [string]::IsNullOrWhiteSpace($PendingPath) -and -not (Test-Path -LiteralPath $PendingPath -PathType Leaf)) {
    throw "Prepare phase claimed pending checkpoint, but file was not written: $PendingPath"
}

Restart-PieFresh -BridgeState $BridgeState -TimeoutSeconds $BridgeTimeoutSeconds
Start-Sleep -Seconds 2

$VerifyResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_self_test" -Params @{ testId = $VerifyTestId } -TimeoutSeconds 240
$VerifyReportPath = Resolve-ReportPathFromText -Text $VerifyResult.fullReport
$VerifyReport = $null
if ($VerifyReportPath -and (Test-Path -LiteralPath $VerifyReportPath -PathType Leaf)) {
    $VerifyReport = Get-Content -LiteralPath $VerifyReportPath -Raw | ConvertFrom-Json
    $CaptureDir = Split-Path -Parent $VerifyReportPath
    if (-not $CaptureId) {
        $CaptureId = [string]$VerifyReport.captureId
    }
}

$PendingCleared = $true
if (-not [string]::IsNullOrWhiteSpace($PendingPath)) {
    $PendingCleared = -not (Test-Path -LiteralPath $PendingPath -PathType Leaf)
}

$OverallPassed = [bool]$PrepareResult.passed -and [bool]$VerifyResult.passed -and $PendingCleared
$Combined = [ordered]@{
    captureId = $CaptureId
    verdict = if ($OverallPassed) { "pass" } else { "fail" }
    prepare = [ordered]@{
        passed = [bool]$PrepareResult.passed
        summary = [string]$PrepareResult.summary
        fullReport = [string]$PrepareResult.fullReport
        reportPath = $PrepareReportPath
    }
    verifyRestore = [ordered]@{
        passed = [bool]$VerifyResult.passed
        summary = [string]$VerifyResult.summary
        fullReport = [string]$VerifyResult.fullReport
        reportPath = $VerifyReportPath
    }
    pendingStatePath = $PendingPath
    pendingCleared = $PendingCleared
    prepareReport = $PrepareReport
    verifyRestoreReport = $VerifyReport
}

New-Item -ItemType Directory -Force -Path $CaptureDir | Out-Null
$CombinedPath = Join-Path $CaptureDir "external_runner.json"
$Combined | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $CombinedPath -Encoding UTF8

Get-Content -LiteralPath $CombinedPath -Raw

if (-not $OverallPassed) {
    exit 1
}
