[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [int]$EditorLaunchTimeoutSeconds = 180
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$BridgeStatePath = Join-Path $ProjectRoot "Saved\UE_MCP_Bridge\bridge_state.json"
$ResultLogPath = Join-Path $ProjectRoot "Saved\open_fab_screenshot_state.log"

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

Remove-Item -LiteralPath $ResultLogPath -Force -ErrorAction SilentlyContinue

$BridgeState = Ensure-EditorReady
$PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" }
if (-not $PieStatus.success) {
    throw "Failed to query PIE status."
}

if (-not $PieStatus.isPlaying) {
    [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "start" })
    $PieDeadline = (Get-Date).AddSeconds(60)
    do {
        Start-Sleep -Seconds 1
        $PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" }
        if ($PieStatus.isPlaying) {
            break
        }
    } while ((Get-Date) -lt $PieDeadline)
    if (-not $PieStatus.isPlaying) {
        throw "Failed to start PIE before applying fab screenshot state."
    }
}

$Result = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_workflow" -Params @{ workflowId = "fab_screenshot_foundation" } -TimeoutSeconds 60
$Summary = "fab_screenshot_foundation={0} | Passed={1} Failed={2} Summary={3}" -f `
    ($(if ($Result.passed) { "PASS" } elseif ($Result.blocked) { "BLOCKED" } else { "FAIL" })), `
    $Result.passedStepCount, `
    $Result.failedStepCount, `
    $Result.summary

$Summary | Tee-Object -FilePath $ResultLogPath -Append | Out-Host
if ($Result.fullReport) {
    $Result.fullReport | Tee-Object -FilePath $ResultLogPath -Append | Out-Host
}

if (-not $Result.passed) {
    throw "fab_screenshot_foundation failed. See log: $ResultLogPath"
}
