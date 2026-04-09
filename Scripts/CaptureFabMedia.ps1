[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Software\Unreal\UE_5.5",
    [string]$OutputRoot = "",
    [switch]$UseProjectFabMediaOutput,
    [int]$EditorLaunchTimeoutSeconds = 180,
    [int]$StepDelaySeconds = 1
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$BridgeStatePath = Join-Path $ProjectRoot "Saved\UE_MCP_Bridge\bridge_state.json"
$DefaultOutputRoot = Join-Path $ProjectRoot "Saved\RuntimeInspector\FabMediaCapture"
$ProjectFabMediaRoot = Join-Path $PluginRoot "FabMedia"
$OutputRoot = if (-not [string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot
} elseif ($UseProjectFabMediaOutput) {
    $ProjectFabMediaRoot
} else {
    $DefaultOutputRoot
}
$CaptureLogPath = Join-Path $OutputRoot "capture_fab_media.log"
$ManifestPath = Join-Path $OutputRoot "capture_manifest.txt"

function Write-Utf8FileLine {
    param(
        [string]$Path,
        [string]$Line
    )

    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
    Add-Content -LiteralPath $Path -Value $Line -Encoding utf8
}

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
        [string]$LeafName,
        [int]$TimeoutSeconds = 15
    )

    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path $Path -PathType Leaf) {
            return $Path
        }

        foreach ($SearchRoot in @(
            (Join-Path $ProjectRoot "Saved\Screenshots"),
            (Join-Path $ProjectRoot "Saved")
        )) {
            if (-not (Test-Path $SearchRoot -PathType Container)) {
                continue
            }

            $Candidate = Get-ChildItem -LiteralPath $SearchRoot -Recurse -File -Filter $LeafName -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($Candidate) {
                if ($Candidate.FullName -ne $Path) {
                    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
                    Copy-Item -LiteralPath $Candidate.FullName -Destination $Path -Force
                }
                return $Path
            }
        }

        Start-Sleep -Seconds 1
    }

    return $null
}

function Get-FabCaptureSelfTestId {
    param(
        [string]$ShotName,
        [string]$PageName
    )

    $Key = ("{0}|{1}" -f $ShotName, $PageName).ToLowerInvariant()
    switch ($Key) {
        "foundation|changes" { return "fab_screenshot_foundation" }
        "foundation|actor" { return "fab_screenshot_actor_page" }
        "foundation|settings" { return "fab_screenshot_settings_page" }
        "foundation|tools" { return "fab_screenshot_tools_page" }
        "remote_session|changes" { return "fab_screenshot_remote_session" }
        "promote_or_audit|changes" { return "fab_screenshot_promote_or_audit" }
        default { throw "No Fab screenshot self-test mapping for shot/page: $ShotName / $PageName" }
    }
}

function Invoke-FabCaptureSelfTest {
    param(
        [string]$ShotName,
        [string]$PageName
    )

    $TestId = Get-FabCaptureSelfTestId -ShotName $ShotName -PageName $PageName
    $Result = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_self_test" -Params @{ testId = $TestId } -TimeoutSeconds 90
    if (-not $Result.success) {
        throw "run_runtime_self_test transport failed for $TestId"
    }

    if ($Result.PSObject.Properties.Match("passed").Count -gt 0 -and -not $Result.passed) {
        $Failure = if ($Result.PSObject.Properties.Match("fullReport").Count -gt 0 -and -not [string]::IsNullOrWhiteSpace($Result.fullReport)) { $Result.fullReport } else { "Unknown self-test failure" }
        throw "$TestId failed: $Failure"
    }

    $Summary = if ($Result.PSObject.Properties.Match("summary").Count -gt 0) { $Result.summary } else { $TestId }
    Write-Host ("{0} -> {1}" -f $TestId, $Summary)
    Start-Sleep -Seconds $StepDelaySeconds
}

function Capture-InspectorShot {
    param(
        [string]$ShotName,
        [string]$PageName,
        [string]$FileName
    )

    Invoke-FabCaptureSelfTest -ShotName $ShotName -PageName $PageName

    $TargetPath = Join-Path $OutputRoot $FileName
    $CaptureResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "capture_window_screenshot" -Params @{
        filename = $TargetPath
        windowTitleContains = "Preview"
    } -TimeoutSeconds 30
    if (-not $CaptureResult.success) {
        throw "capture_window_screenshot failed for $FileName"
    }

    $ResolvedPath = if ($CaptureResult.PSObject.Properties.Match("filename").Count -gt 0 -and -not [string]::IsNullOrWhiteSpace($CaptureResult.filename)) {
        [string]$CaptureResult.filename
    } else {
        $TargetPath
    }
    $ResolvedPath = Wait-ForFile -Path $ResolvedPath -LeafName $FileName -TimeoutSeconds 5
    if (-not $ResolvedPath) {
        throw "Screenshot file was not produced: $FileName"
    }

    $Line = "{0} -> {1}" -f $FileName, $ResolvedPath
    Write-Utf8FileLine -Path $ManifestPath -Line $Line
    Write-Host $Line
    return $ResolvedPath
}

New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
Remove-Item -LiteralPath $CaptureLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $ManifestPath -Force -ErrorAction SilentlyContinue

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
        throw "Failed to start PIE before capturing Fab media."
    }
}

$Shots = @(
    @{ File = "cover.png"; Shot = "foundation"; Page = "Changes" },
    @{ File = "screenshot_01_actor_panel.png"; Shot = "foundation"; Page = "Actor" },
    @{ File = "screenshot_02_changes_workflow.png"; Shot = "foundation"; Page = "Changes" },
    @{ File = "screenshot_03_settings.png"; Shot = "foundation"; Page = "Settings" },
    @{ File = "screenshot_04_tools.png"; Shot = "foundation"; Page = "Tools" },
    @{ File = "screenshot_05_remote_session.png"; Shot = "remote_session"; Page = "Changes" },
    @{ File = "screenshot_06_promote_or_audit.png"; Shot = "promote_or_audit"; Page = "Changes" }
)

$Results = @()
foreach ($Shot in $Shots) {
    $Resolved = Capture-InspectorShot -ShotName $Shot.Shot -PageName $Shot.Page -FileName $Shot.File
    $Results += [pscustomobject]@{
        file = $Shot.File
        shot = $Shot.Shot
        page = $Shot.Page
        testId = Get-FabCaptureSelfTestId -ShotName $Shot.Shot -PageName $Shot.Page
        path = $Resolved
    }
}

$SummaryLine = "Fab media capture complete | OutputRoot={0} | Shots={1}" -f $OutputRoot, $Results.Count
Write-Utf8FileLine -Path $CaptureLogPath -Line $SummaryLine
Write-Host $SummaryLine
$ResultsJson = $Results | ConvertTo-Json -Depth 4
Write-Utf8FileLine -Path $CaptureLogPath -Line $ResultsJson
Write-Host $ResultsJson
Write-Host "Captured files:"
foreach ($Result in $Results) {
    Write-Host ("- {0}" -f $Result.path)
}
Write-Host "OutputRoot: $OutputRoot"
Write-Host "Log: $CaptureLogPath"
Write-Host "Manifest: $ManifestPath"
