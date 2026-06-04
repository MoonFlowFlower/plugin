[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Software\Unreal\UE_5.5",
    [string]$OutputRoot = "",
    [switch]$UseProjectFabMediaOutput,
    [switch]$IncludeOptionalShots,
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

function Get-BridgeFieldValue {
    param(
        [object]$BridgeResult,
        [string]$FieldName
    )

    if ($null -eq $BridgeResult -or [string]::IsNullOrWhiteSpace($FieldName)) {
        return $null
    }

    $RawValue = $BridgeResult.$FieldName
    if ($null -eq $RawValue) {
        return $null
    }

    if ($RawValue -is [System.Array]) {
        foreach ($Entry in $RawValue) {
            if ($null -eq $Entry) {
                continue
            }

            if ($Entry -is [string]) {
                if (-not [string]::IsNullOrWhiteSpace($Entry)) {
                    return $Entry
                }
                continue
            }

            return $Entry
        }

        return $null
    }

    return $RawValue
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

function Ensure-NativeWindowCaptureType {
    try {
        [RuntimeInspector.NativeWindowCapture] | Out-Null
        return
    } catch {
    }

    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace RuntimeInspector
{
public class NativeWindowCapture
{
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool BringWindowToTop(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    public static IntPtr FindWindowContaining(string needle, out string title)
    {
        title = String.Empty;
        IntPtr found = IntPtr.Zero;
        string foundTitle = String.Empty;
        long foundArea = 0;
        EnumWindows(delegate(IntPtr hWnd, IntPtr lParam)
        {
            if (!IsWindowVisible(hWnd))
            {
                return true;
            }

            StringBuilder builder = new StringBuilder(512);
            GetWindowText(hWnd, builder, builder.Capacity);
            string currentTitle = builder.ToString();
            if (String.IsNullOrWhiteSpace(currentTitle))
            {
                return true;
            }

            if (currentTitle.IndexOf(needle, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                RECT rect;
                if (!GetWindowRect(hWnd, out rect))
                {
                    return true;
                }

                int width = rect.Right - rect.Left;
                int height = rect.Bottom - rect.Top;
                if (width < 300 || height < 200)
                {
                    return true;
                }

                long area = (long)width * (long)height;
                if (area > foundArea)
                {
                    found = hWnd;
                    foundTitle = currentTitle;
                    foundArea = area;
                }
            }

            return true;
        }, IntPtr.Zero);

        title = foundTitle;
        return found;
    }
}
}
'@
}

function Save-NativeWindowScreenshot {
    param(
        [string]$WindowTitleContains,
        [string]$TargetPath
    )

    Ensure-NativeWindowCaptureType
    Add-Type -AssemblyName System.Drawing

    $FoundTitle = ""
    $WindowHandle = [RuntimeInspector.NativeWindowCapture]::FindWindowContaining($WindowTitleContains, [ref]$FoundTitle)
    if ($WindowHandle -eq [IntPtr]::Zero) {
        throw "Could not find visible window containing title: $WindowTitleContains"
    }

    $Rect = New-Object RuntimeInspector.NativeWindowCapture+RECT
    if (-not [RuntimeInspector.NativeWindowCapture]::GetWindowRect($WindowHandle, [ref]$Rect)) {
        throw "Could not read window bounds for: $FoundTitle"
    }

    $Width = $Rect.Right - $Rect.Left
    $Height = $Rect.Bottom - $Rect.Top
    if ($Width -le 0 -or $Height -le 0) {
        throw "Invalid window bounds for $FoundTitle : ${Width}x${Height}"
    }

    $HWND_TOPMOST = [IntPtr](-1)
    $HWND_NOTOPMOST = [IntPtr](-2)
    $SWP_NOMOVE = 0x0002
    $SWP_NOSIZE = 0x0001
    $SWP_SHOWWINDOW = 0x0040
    $WindowPlacementFlags = $SWP_NOMOVE -bor $SWP_NOSIZE -bor $SWP_SHOWWINDOW

    [void][RuntimeInspector.NativeWindowCapture]::ShowWindow($WindowHandle, 9)
    [void][RuntimeInspector.NativeWindowCapture]::SetWindowPos($WindowHandle, $HWND_TOPMOST, 0, 0, 0, 0, $WindowPlacementFlags)
    [void][RuntimeInspector.NativeWindowCapture]::BringWindowToTop($WindowHandle)
    [void][RuntimeInspector.NativeWindowCapture]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 500

    New-Item -ItemType Directory -Path (Split-Path -Parent $TargetPath) -Force | Out-Null
    $Bitmap = New-Object System.Drawing.Bitmap $Width, $Height
    $Graphics = [System.Drawing.Graphics]::FromImage($Bitmap)
    try {
        $Graphics.CopyFromScreen($Rect.Left, $Rect.Top, 0, 0, $Bitmap.Size)
        $Bitmap.Save($TargetPath, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $Graphics.Dispose()
        $Bitmap.Dispose()
        [void][RuntimeInspector.NativeWindowCapture]::SetWindowPos($WindowHandle, $HWND_NOTOPMOST, 0, 0, 0, 0, $WindowPlacementFlags)
    }

    return [pscustomobject]@{
        title = $FoundTitle
        width = $Width
        height = $Height
        path = $TargetPath
    }
}

function Get-InspectorAutomationSummary {
    param(
        [object]$BridgeState,
        [int]$AttemptCount = 24
    )

    $LastSummary = $null
    for ($Attempt = 1; $Attempt -le $AttemptCount; $Attempt++) {
        $LastSummary = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary" -TimeoutSeconds 30
        $SummarySucceeded = [bool](Get-BridgeFieldValue -BridgeResult $LastSummary -FieldName "success")
        $RawDebug = [string](Get-BridgeFieldValue -BridgeResult $LastSummary -FieldName "panelHostWindowDebug")
        $WindowTitle = Get-WindowTitleFromSummary -Summary $RawDebug
        if ($SummarySucceeded -and -not [string]::IsNullOrWhiteSpace($WindowTitle)) {
            return $LastSummary
        }

        Start-Sleep -Milliseconds 500
    }

    return $LastSummary
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

function Invoke-ActorPagePickForCapture {
    $PositionResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "position_mouse_on_player_character" } -TimeoutSeconds 30
    if (-not [bool](Get-BridgeFieldValue -BridgeResult $PositionResult -FieldName "success")) {
        $ErrorText = [string](Get-BridgeFieldValue -BridgeResult $PositionResult -FieldName "error")
        throw "Failed to position mouse on player character before actor screenshot: $ErrorText"
    }

    $PickResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "right_mouse_pick_input" } -TimeoutSeconds 30
    if (-not [bool](Get-BridgeFieldValue -BridgeResult $PickResult -FieldName "success")) {
        $ErrorText = [string](Get-BridgeFieldValue -BridgeResult $PickResult -FieldName "error")
        throw "Failed to run actor screenshot pick input: $ErrorText"
    }
    if (-not [bool](Get-BridgeFieldValue -BridgeResult $PickResult -FieldName "inputHandled")) {
        $DebugText = [string](Get-BridgeFieldValue -BridgeResult $PickResult -FieldName "lastPickDebug")
        throw "Actor screenshot pick input was not handled: $DebugText"
    }

    Write-Host ("Actor screenshot pick -> {0}" -f ([string](Get-BridgeFieldValue -BridgeResult $PickResult -FieldName "lastPickDebug")))
    Start-Sleep -Milliseconds 500
}

function Capture-InspectorShot {
    param(
        [string]$ShotName,
        [string]$PageName,
        [string]$FileName
    )

    if ($ShotName -eq "foundation" -and $PageName -eq "Actor") {
        Invoke-ActorPagePickForCapture
    }
    Invoke-FabCaptureSelfTest -ShotName $ShotName -PageName $PageName
    Start-Sleep -Milliseconds 750
    $DetectedHostWindowTitle = ""
    $HostWindowTitle = "PluginMaker Preview"

    $TargetPath = Join-Path $OutputRoot $FileName
    $CaptureResult = Save-NativeWindowScreenshot -WindowTitleContains $HostWindowTitle -TargetPath $TargetPath
    $ResolvedPath = Wait-ForFile -Path $CaptureResult.path -LeafName $FileName -TimeoutSeconds 5
    if (-not $ResolvedPath) {
        throw "Screenshot file was not produced: $FileName"
    }

    $Line = "{0} -> {1}" -f $FileName, $ResolvedPath
    Write-Utf8FileLine -Path $ManifestPath -Line $Line
    Write-Host $Line
    Write-Host ("Captured window: {0} ({1}x{2})" -f $CaptureResult.title, $CaptureResult.width, $CaptureResult.height)
    if (-not [string]::IsNullOrWhiteSpace($DetectedHostWindowTitle) -and $DetectedHostWindowTitle -ne $HostWindowTitle) {
        Write-Host ("Detected host title: {0}" -f $DetectedHostWindowTitle)
    }
    return $ResolvedPath
}

New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
Remove-Item -LiteralPath $CaptureLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $ManifestPath -Force -ErrorAction SilentlyContinue

$BridgeState = Ensure-EditorReady
$PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" } -TimeoutSeconds 90
if (-not $PieStatus.success) {
    throw "Failed to query PIE status."
}

if (-not $PieStatus.isPlaying) {
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
        throw "Failed to start PIE before capturing Fab media."
    }
}

$Shots = @(
    @{ File = "cover.png"; Shot = "foundation"; Page = "Changes" },
    @{ File = "screenshot_01_actor_panel.png"; Shot = "foundation"; Page = "Actor" },
    @{ File = "screenshot_02_changes_workflow.png"; Shot = "foundation"; Page = "Changes" },
    @{ File = "screenshot_03_settings.png"; Shot = "foundation"; Page = "Settings" },
    @{ File = "screenshot_04_tools.png"; Shot = "foundation"; Page = "Tools" }
)

if ($IncludeOptionalShots) {
    $Shots += @(
        @{ File = "screenshot_05_remote_session.png"; Shot = "remote_session"; Page = "Changes" },
        @{ File = "screenshot_06_promote_or_audit.png"; Shot = "promote_or_audit"; Page = "Changes" }
    )
}

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
