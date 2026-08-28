[CmdletBinding()]
param(
    [string]$ProjectRoot = "",
    [string]$HostLabel = "PluginMakerHorizontal",
    [string]$OutputDir = "",
    [string[]]$Resolutions = @("1280x720", "1600x900", "1920x1080", "2560x1440", "3840x2160", "900x1200"),
    [double[]]$UIScales = @(0.8, 1.0, 1.25, 1.5),
    [switch]$CaptureScreenshots,
    [switch]$RunRealMouseChecks,
    [int]$BridgeTimeoutSeconds = 120
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
} else {
    $ProjectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)
}
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $ProjectRoot ("Saved\RuntimeInspector\ResponsiveDPI\Matrix\{0}" -f $HostLabel)
}
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$BridgeStatePath = Join-Path $ProjectRoot "Saved\UE_MCP_Bridge\bridge_state.json"
$ReportPath = Join-Path $OutputDir "responsive-dpi-matrix.json"
$ScreenshotDir = Join-Path $OutputDir "Screenshots"

function Test-TcpPort {
    param([int]$Port, [int]$TimeoutMs = 250)
    $Client = [System.Net.Sockets.TcpClient]::new()
    try {
        $Async = $Client.BeginConnect("127.0.0.1", $Port, $null, $null)
        if (-not $Async.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) {
            return $false
        }
        $Client.EndConnect($Async)
        return $true
    } catch {
        return $false
    } finally {
        $Client.Dispose()
    }
}

function Get-BridgeState {
    if (-not (Test-Path -LiteralPath $BridgeStatePath -PathType Leaf)) {
        throw "UE bridge state not found: $BridgeStatePath"
    }
    $State = Get-Content -LiteralPath $BridgeStatePath -Raw | ConvertFrom-Json
    if (-not $State.active -or -not $State.port -or -not (Test-TcpPort -Port ([int]$State.port))) {
        throw "UE bridge is not active: $BridgeStatePath"
    }
    return $State
}

function Invoke-BridgeRequest {
    param(
        [Parameter(Mandatory = $true)][object]$BridgeState,
        [Parameter(Mandatory = $true)][string]$Method,
        [hashtable]$Params = @{},
        [int]$TimeoutSeconds = $BridgeTimeoutSeconds
    )

    $Socket = [System.Net.WebSockets.ClientWebSocket]::new()
    $Cancellation = [System.Threading.CancellationTokenSource]::new()
    $Cancellation.CancelAfter($TimeoutSeconds * 1000)
    try {
        $Uri = [Uri]::new(("ws://127.0.0.1:{0}" -f [int]$BridgeState.port))
        $Socket.ConnectAsync($Uri, $Cancellation.Token).GetAwaiter().GetResult() | Out-Null
        $Payload = @{
            jsonrpc = "2.0"
            id = [Guid]::NewGuid().ToString("N")
            method = $Method
            params = $Params
        } | ConvertTo-Json -Depth 12 -Compress
        $Bytes = [System.Text.Encoding]::UTF8.GetBytes($Payload)
        $Segment = [System.ArraySegment[byte]]::new($Bytes)
        $Socket.SendAsync($Segment, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, $Cancellation.Token).GetAwaiter().GetResult() | Out-Null

        $Buffer = New-Object byte[] 65536
        $Builder = [System.Text.StringBuilder]::new()
        do {
            $ReceiveSegment = [System.ArraySegment[byte]]::new($Buffer)
            $Receive = $Socket.ReceiveAsync($ReceiveSegment, $Cancellation.Token).GetAwaiter().GetResult()
            if ($Receive.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) {
                throw "Bridge closed before returning a result for '$Method'."
            }
            if ($Receive.Count -gt 0) {
                [void]$Builder.Append([System.Text.Encoding]::UTF8.GetString($Buffer, 0, $Receive.Count))
            }
        } while (-not $Receive.EndOfMessage)

        $Response = $Builder.ToString() | ConvertFrom-Json
        if ($Response.PSObject.Properties["error"] -and $Response.error) {
            throw ([string]$Response.error.message)
        }
        return $Response.result
    } finally {
        $Socket.Dispose()
        $Cancellation.Dispose()
    }
}

function Get-ReportMetric {
    param([string]$Report, [string]$Key)
    $Match = [regex]::Match($Report, ("(?:^|\s){0}=([-+]?\d+(?:\.\d+)?)" -f [regex]::Escape($Key)))
    if (-not $Match.Success) {
        throw "Metric '$Key' missing from report: $Report"
    }
    return [double]::Parse($Match.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
}

function Get-CurrentLayout {
    param([object]$BridgeState)
    $Summary = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary"
    $Debug = [string]$Summary.panelHostWindowDebug
    if (-not $Debug.StartsWith("DockRoot=1")) {
        throw "Native dock debug summary unavailable: $Debug"
    }
    return [pscustomobject]@{
        debug = $Debug
        physicalWidth = Get-ReportMetric -Report $Debug -Key "PhysicalWidth"
        physicalHeight = Get-ReportMetric -Report $Debug -Key "PhysicalHeight"
        userScale = Get-ReportMetric -Report $Debug -Key "UserScale"
    }
}

function Set-RuntimeUIScale {
    param([object]$BridgeState, [double]$Scale)
    $InvariantScale = $Scale.ToString("0.###", [Globalization.CultureInfo]::InvariantCulture)
    $Code = @"
import unreal
settings_class = unreal.load_class(None, '/Script/RuntimeInspector.RuntimeInspectorSettings')
settings = unreal.get_default_object(settings_class)
settings.set_editor_property('UIScale', $InvariantScale)
"@
    $Result = Invoke-BridgeRequest -BridgeState $BridgeState -Method "execute_python" -Params @{ code = $Code }
    if (-not $Result.success) {
        throw "Failed to set runtime UIScale=$InvariantScale : $($Result.result)"
    }
}

function Ensure-ResponsiveWindowType {
    if ("RuntimeInspector.ResponsiveDpiWindow" -as [type]) {
        return
    }
    Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace RuntimeInspector
{
    public static class ResponsiveDpiWindow
    {
        public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
        [StructLayout(LayoutKind.Sequential)]
        public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }

        [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);
        [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int count);
        [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
        [DllImport("user32.dll", EntryPoint = "GetWindowLongW")] public static extern int GetWindowLong(IntPtr hWnd, int index);
        [DllImport("user32.dll", EntryPoint = "SetWindowLongW")] public static extern int SetWindowLong(IntPtr hWnd, int index, int newValue);
        [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int command);
        [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr insertAfter, int x, int y, int width, int height, uint flags);
        [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
        [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint x, uint y, uint data, UIntPtr extraInfo);
        [DllImport("user32.dll")] public static extern void keybd_event(byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);

        public static IntPtr FindLargestPreviewWindow()
        {
            IntPtr best = IntPtr.Zero;
            long bestArea = 0;
            EnumWindows(delegate(IntPtr hWnd, IntPtr lParam)
            {
                if (!IsWindowVisible(hWnd)) return true;
                StringBuilder title = new StringBuilder(512);
                GetWindowText(hWnd, title, title.Capacity);
                string value = title.ToString();
                if (value.IndexOf("Preview [NetMode", StringComparison.OrdinalIgnoreCase) < 0) return true;
                RECT rect;
                if (!GetWindowRect(hWnd, out rect)) return true;
                long area = (long)(rect.Right - rect.Left) * (long)(rect.Bottom - rect.Top);
                if (area > bestArea) { best = hWnd; bestArea = area; }
                return true;
            }, IntPtr.Zero);
            return best;
        }
    }
}
'@
}

function Set-ValidationWindowBorderless {
    Ensure-ResponsiveWindowType
    $Window = [RuntimeInspector.ResponsiveDpiWindow]::FindLargestPreviewWindow()
    if ($Window -eq [IntPtr]::Zero) {
        throw "Could not locate the standalone PIE preview window."
    }
    $Style = [RuntimeInspector.ResponsiveDpiWindow]::GetWindowLong($Window, -16)
    $BorderMask = 0x00C00000 -bor 0x00040000 -bor 0x00080000 -bor 0x00020000 -bor 0x00010000
    $BorderlessStyle = $Style -band (-bnot $BorderMask)
    [void][RuntimeInspector.ResponsiveDpiWindow]::SetWindowLong($Window, -16, $BorderlessStyle)
    $FrameFlags = 0x0001 -bor 0x0002 -bor 0x0004 -bor 0x0020 -bor 0x0040
    [void][RuntimeInspector.ResponsiveDpiWindow]::SetWindowPos($Window, [IntPtr]::Zero, 0, 0, 0, 0, $FrameFlags)
    return [pscustomobject]@{ window = $Window; style = $Style }
}

function Restore-ValidationWindowStyle {
    param([IntPtr]$Window, [int]$Style)
    [void][RuntimeInspector.ResponsiveDpiWindow]::SetWindowLong($Window, -16, $Style)
    $FrameFlags = 0x0001 -bor 0x0002 -bor 0x0004 -bor 0x0020 -bor 0x0040
    [void][RuntimeInspector.ResponsiveDpiWindow]::SetWindowPos($Window, [IntPtr]::Zero, 0, 0, 0, 0, $FrameFlags)
}

function Set-ExactViewportSize {
    param(
        [object]$BridgeState,
        [int]$Width,
        [int]$Height
    )
    Ensure-ResponsiveWindowType
    $Window = [RuntimeInspector.ResponsiveDpiWindow]::FindLargestPreviewWindow()
    if ($Window -eq [IntPtr]::Zero) {
        throw "Could not locate the standalone PIE preview window."
    }

    [void][RuntimeInspector.ResponsiveDpiWindow]::ShowWindow($Window, 9)
    for ($Attempt = 1; $Attempt -le 8; $Attempt++) {
        $Layout = Get-CurrentLayout -BridgeState $BridgeState
        if ([Math]::Abs($Layout.physicalWidth - $Width) -le 1.0 -and [Math]::Abs($Layout.physicalHeight - $Height) -le 1.0) {
            return $Layout
        }

        $Rect = New-Object RuntimeInspector.ResponsiveDpiWindow+RECT
        if (-not [RuntimeInspector.ResponsiveDpiWindow]::GetWindowRect($Window, [ref]$Rect)) {
            throw "Could not query PIE preview window bounds."
        }
        $OuterWidth = $Rect.Right - $Rect.Left
        $OuterHeight = $Rect.Bottom - $Rect.Top
        $NewWidth = [Math]::Max(320, [int][Math]::Round($OuterWidth + ($Width - $Layout.physicalWidth)))
        $NewHeight = [Math]::Max(240, [int][Math]::Round($OuterHeight + ($Height - $Layout.physicalHeight)))
        $Flags = 0x0002 -bor 0x0004 -bor 0x0040
        if (-not [RuntimeInspector.ResponsiveDpiWindow]::SetWindowPos($Window, [IntPtr]::Zero, 0, 0, $NewWidth, $NewHeight, $Flags)) {
            throw "Could not resize PIE preview window."
        }
        Start-Sleep -Milliseconds 500
    }

    $FinalLayout = Get-CurrentLayout -BridgeState $BridgeState
    throw "Viewport failed to converge to ${Width}x${Height}; actual=$($FinalLayout.physicalWidth)x$($FinalLayout.physicalHeight)."
}

function Wait-ForFile {
    param([string]$Path, [int]$TimeoutSeconds = 15)
    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $Deadline) {
        if (Test-Path -LiteralPath $Path -PathType Leaf) {
            return $true
        }
        Start-Sleep -Milliseconds 250
    }
    return $false
}

function Invoke-RealToggleKey {
    param([object]$BridgeState)
    Ensure-ResponsiveWindowType
    [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "close" })
    Start-Sleep -Milliseconds 250
    $Window = [RuntimeInspector.ResponsiveDpiWindow]::FindLargestPreviewWindow()
    [void][RuntimeInspector.ResponsiveDpiWindow]::ShowWindow($Window, 9)
    [void][RuntimeInspector.ResponsiveDpiWindow]::SetForegroundWindow($Window)
    $Rect = New-Object RuntimeInspector.ResponsiveDpiWindow+RECT
    if (-not [RuntimeInspector.ResponsiveDpiWindow]::GetWindowRect($Window, [ref]$Rect)) {
        throw "Could not query PIE preview window bounds before the O-key check."
    }
    $FocusX = [int][Math]::Round(($Rect.Left + $Rect.Right) / 2.0)
    $FocusY = [int][Math]::Round(($Rect.Top + $Rect.Bottom) / 2.0)
    $AttemptCount = 0
    $PollCount = 0
    $Passed = $false
    $Summary = $null
    do {
        $AttemptCount++
        [void][RuntimeInspector.ResponsiveDpiWindow]::ShowWindow($Window, 9)
        [void][RuntimeInspector.ResponsiveDpiWindow]::SetForegroundWindow($Window)
        [void][RuntimeInspector.ResponsiveDpiWindow]::SetCursorPos($FocusX, $FocusY)
        Start-Sleep -Milliseconds 100
        [RuntimeInspector.ResponsiveDpiWindow]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 60
        [RuntimeInspector.ResponsiveDpiWindow]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 250

        # Query before every retry so a delayed open is never toggled closed by the next O key.
        $Summary = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary"
        $Passed = ([string]$Summary.panelHostWindowDebug).StartsWith("DockRoot=1")
        if ($Passed) {
            break
        }

        [RuntimeInspector.ResponsiveDpiWindow]::keybd_event(0x4F, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 60
        [RuntimeInspector.ResponsiveDpiWindow]::keybd_event(0x4F, 0, 0x0002, [UIntPtr]::Zero)
        for ($Poll = 0; $Poll -lt 4 -and -not $Passed; $Poll++) {
            $PollCount++
            Start-Sleep -Milliseconds 300
            $Summary = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary"
            $Passed = ([string]$Summary.panelHostWindowDebug).StartsWith("DockRoot=1")
        }
    } while (-not $Passed -and $AttemptCount -lt 4)
    if (-not $Passed) {
        [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "open" })
        Start-Sleep -Milliseconds 400
    }
    return [pscustomobject]@{
        name = "PIE O-key open"
        passed = $Passed
        attempts = $AttemptCount
        polls = $PollCount
        detail = [string]$Summary.panelHostWindowDebug
    }
}

function Invoke-RealTabClicks {
    param([object]$BridgeState, [string]$Resolution)
    Ensure-ResponsiveWindowType
    $Window = [RuntimeInspector.ResponsiveDpiWindow]::FindLargestPreviewWindow()
    $ExpectedTabs = [ordered]@{ Actor = 0; Changes = 1; Settings = 2; Tools = 3 }
    $Results = [System.Collections.Generic.List[object]]::new()
    foreach ($Entry in $ExpectedTabs.GetEnumerator()) {
        $Before = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary"
        $Debug = [string]$Before.panelHostWindowDebug
        $CenterMatch = [regex]::Match($Debug, ("(?:TabCenters=|;){0}:([-+]?\d+(?:\.\d+)?),([-+]?\d+(?:\.\d+)?)" -f [regex]::Escape([string]$Entry.Key)))
        if (-not $CenterMatch.Success) {
            $Results.Add([pscustomobject]@{ name = "$Resolution $($Entry.Key) mouse click"; passed = $false; detail = "Tab center unavailable: $Debug" })
            continue
        }

        $X = [int][Math]::Round([double]::Parse($CenterMatch.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture))
        $Y = [int][Math]::Round([double]::Parse($CenterMatch.Groups[2].Value, [Globalization.CultureInfo]::InvariantCulture))
        $AttemptCount = 0
        do {
            $AttemptCount++
            [void][RuntimeInspector.ResponsiveDpiWindow]::ShowWindow($Window, 9)
            [void][RuntimeInspector.ResponsiveDpiWindow]::SetForegroundWindow($Window)
            [void][RuntimeInspector.ResponsiveDpiWindow]::SetCursorPos($X, $Y)
            Start-Sleep -Milliseconds 150
            [RuntimeInspector.ResponsiveDpiWindow]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 60
            [RuntimeInspector.ResponsiveDpiWindow]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 350

            $After = Invoke-BridgeRequest -BridgeState $BridgeState -Method "get_runtime_inspector_automation_summary"
            $AfterDebug = [string]$After.panelHostWindowDebug
            $ActiveTab = Get-ReportMetric -Report $AfterDebug -Key "ActiveTab"
            $Passed = [int]$ActiveTab -eq [int]$Entry.Value
        } while (-not $Passed -and $AttemptCount -lt 2)
        $Results.Add([pscustomobject]@{
            name = "$Resolution $($Entry.Key) mouse click"
            passed = $Passed
            x = $X
            y = $Y
            expectedTab = [int]$Entry.Value
            actualTab = [int]$ActiveTab
            attempts = $AttemptCount
            detail = $AfterDebug
        })
    }
    return $Results
}

function Test-MetricSpread {
    param([object[]]$Rows, [string]$Metric)
    $Values = @($Rows | ForEach-Object { [double]($_.$Metric) })
    $Minimum = ($Values | Measure-Object -Minimum).Minimum
    $Maximum = ($Values | Measure-Object -Maximum).Maximum
    $Tolerance = [Math]::Max(1.0, [Math]::Abs([double]$Minimum) * 0.05)
    return [pscustomobject]@{
        metric = $Metric
        minimum = $Minimum
        maximum = $Maximum
        tolerance = $Tolerance
        passed = (($Maximum - $Minimum) -le $Tolerance)
    }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
if ($CaptureScreenshots) {
    New-Item -ItemType Directory -Force -Path $ScreenshotDir | Out-Null
}

$BridgeState = Get-BridgeState
$PieStatus = Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "status" }
if (-not $PieStatus.isPlaying) {
    [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "pie_control" -Params @{ action = "start" })
    Start-Sleep -Seconds 2
}
[void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "open" })
Start-Sleep -Seconds 1

$OriginalLayout = Get-CurrentLayout -BridgeState $BridgeState
$OriginalWindow = Set-ValidationWindowBorderless
$WindowStyleRestored = $false
$Rows = [System.Collections.Generic.List[object]]::new()
$Checks = [System.Collections.Generic.List[object]]::new()
$RealMouseChecks = [System.Collections.Generic.List[object]]::new()
$RunError = $null

try {
    foreach ($Resolution in $Resolutions) {
        if ($Resolution -notmatch '^(\d+)x(\d+)$') {
            throw "Invalid resolution '$Resolution'; expected WIDTHxHEIGHT."
        }
        $Width = [int]$Matches[1]
        $Height = [int]$Matches[2]
        foreach ($Scale in $UIScales) {
            Set-RuntimeUIScale -BridgeState $BridgeState -Scale $Scale
            [void](Set-ExactViewportSize -BridgeState $BridgeState -Width $Width -Height $Height)
            Start-Sleep -Milliseconds 350

            $TestResult = Invoke-BridgeRequest -BridgeState $BridgeState -Method "run_runtime_self_test" -Params @{ testId = "responsive_dpi_layout" }
            $FullReport = [string]$TestResult.fullReport
            if ($RunRealMouseChecks -and [Math]::Abs([double]$Scale - 1.0) -le 0.001 -and $Resolution -in @("1280x720", "900x1200")) {
                if ($Resolution -eq "1280x720") {
                    $RealMouseChecks.Add((Invoke-RealToggleKey -BridgeState $BridgeState))
                }
                foreach ($MouseCheck in @(Invoke-RealTabClicks -BridgeState $BridgeState -Resolution $Resolution)) {
                    $RealMouseChecks.Add($MouseCheck)
                }
            }
            $ScreenshotPath = ""
            $ScreenshotReady = $false
            if ($CaptureScreenshots) {
                $ScaleSlug = ([double]$Scale).ToString("0.00", [Globalization.CultureInfo]::InvariantCulture).Replace('.', '_')
                $ScreenshotPath = Join-Path $ScreenshotDir ("{0}_{1}_scale_{2}.png" -f $HostLabel, $Resolution, $ScaleSlug)
                Remove-Item -LiteralPath $ScreenshotPath -Force -ErrorAction SilentlyContinue
                $Capture = Invoke-BridgeRequest -BridgeState $BridgeState -Method "capture_screenshot" -Params @{
                    filename = $ScreenshotPath
                    showUI = $true
                    scope = "viewport"
                }
                $ScreenshotReady = [bool]$Capture.success -and (Wait-ForFile -Path $ScreenshotPath)
            }

            $Row = [ordered]@{
                resolution = $Resolution
                requestedWidth = $Width
                requestedHeight = $Height
                uiScale = [double]$Scale
                passed = [bool]$TestResult.passed
                viewportWidth = Get-ReportMetric -Report $FullReport -Key "PhysicalWidth"
                viewportHeight = Get-ReportMetric -Report $FullReport -Key "PhysicalHeight"
                hostViewportDpi = Get-ReportMetric -Report $FullReport -Key "ViewportScale"
                effectiveContentScale = Get-ReportMetric -Report $FullReport -Key "ContentScale"
                tabHeightPhysical = Get-ReportMetric -Report $FullReport -Key "TabHeightPhysical"
                titleTokenPhysical = Get-ReportMetric -Report $FullReport -Key "TitleTokenPhysical"
                labelTokenPhysical = Get-ReportMetric -Report $FullReport -Key "LabelTokenPhysical"
                mutedTokenPhysical = Get-ReportMetric -Report $FullReport -Key "MutedTokenPhysical"
                controlTokenPhysical = Get-ReportMetric -Report $FullReport -Key "ControlTokenPhysical"
                leftPanelPhysical = Get-ReportMetric -Report $FullReport -Key "LeftActualPhysical"
                rightPanelPhysical = Get-ReportMetric -Report $FullReport -Key "RightActualPhysical"
                centerPhysical = Get-ReportMetric -Report $FullReport -Key "CenterActualPhysical"
                screenshotPath = $ScreenshotPath
                screenshotReady = $ScreenshotReady
                report = $FullReport
            }
            $Rows.Add([pscustomobject]$Row)
            Write-Host ("[{0}] {1} scale={2:0.00} dpi={3:0.###} content={4:0.###}" -f $(if ($Row.passed) { "PASS" } else { "FAIL" }), $Resolution, $Scale, $Row.hostViewportDpi, $Row.effectiveContentScale)
        }
    }

    foreach ($Scale in $UIScales) {
        $ScaleRows = @($Rows | Where-Object { [Math]::Abs($_.uiScale - $Scale) -le 0.001 })
        foreach ($Metric in @("tabHeightPhysical", "titleTokenPhysical", "labelTokenPhysical", "mutedTokenPhysical", "controlTokenPhysical")) {
            $Check = Test-MetricSpread -Rows $ScaleRows -Metric $Metric
            $Checks.Add([pscustomobject]@{
                uiScale = [double]$Scale
                metric = $Check.metric
                minimum = $Check.minimum
                maximum = $Check.maximum
                tolerance = $Check.tolerance
                passed = $Check.passed
            })
        }
    }
} catch {
    $RunError = $_.Exception.Message
} finally {
    try {
        [void](Invoke-BridgeRequest -BridgeState $BridgeState -Method "control_runtime_inspector" -Params @{ action = "open" })
        Start-Sleep -Milliseconds 200
        Set-RuntimeUIScale -BridgeState $BridgeState -Scale $OriginalLayout.userScale
        Restore-ValidationWindowStyle -Window $OriginalWindow.window -Style $OriginalWindow.style
        $WindowStyleRestored = $true
        [void](Set-ExactViewportSize -BridgeState $BridgeState -Width ([int]$OriginalLayout.physicalWidth) -Height ([int]$OriginalLayout.physicalHeight))
    } catch {
        if ([string]::IsNullOrWhiteSpace($RunError)) {
            $RunError = "Restore failed: $($_.Exception.Message)"
        } else {
            $RunError += " | Restore failed: $($_.Exception.Message)"
        }
    }
}

$AllRowsPassed = $Rows.Count -eq ($Resolutions.Count * $UIScales.Count) -and @($Rows | Where-Object { -not $_.passed }).Count -eq 0
$AllChecksPassed = $Checks.Count -gt 0 -and @($Checks | Where-Object { -not $_.passed }).Count -eq 0
$AllScreenshotsReady = -not $CaptureScreenshots -or @($Rows | Where-Object { -not $_.screenshotReady }).Count -eq 0
$AllRealMouseChecksPassed = -not $RunRealMouseChecks -or ($RealMouseChecks.Count -eq 9 -and @($RealMouseChecks | Where-Object { -not $_.passed }).Count -eq 0)
$Passed = [string]::IsNullOrWhiteSpace($RunError) -and $AllRowsPassed -and $AllChecksPassed -and $AllScreenshotsReady -and $AllRealMouseChecksPassed

$Report = [ordered]@{
    schema = "runtimeinspector.responsive-dpi-matrix.v1"
    generatedUtc = [DateTime]::UtcNow.ToString("o")
    passed = $Passed
    hostLabel = $HostLabel
    projectRoot = $ProjectRoot
    bridgePort = [int]$BridgeState.port
    captureScreenshots = [bool]$CaptureScreenshots
    runRealMouseChecks = [bool]$RunRealMouseChecks
    resolutions = $Resolutions
    uiScales = $UIScales
    originalLayout = $OriginalLayout
    windowStyleRestored = $WindowStyleRestored
    rows = $Rows
    invariantChecks = $Checks
    realMouseChecks = $RealMouseChecks
    error = $RunError
}
$Report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $ReportPath -Encoding UTF8

if (-not $Passed) {
    Write-Host "[FAIL] Responsive DPI matrix: $ReportPath" -ForegroundColor Red
    if (-not [string]::IsNullOrWhiteSpace($RunError)) {
        Write-Host "       $RunError" -ForegroundColor Red
    }
    exit 2
}

Write-Host "[PASS] Responsive DPI matrix: $ReportPath" -ForegroundColor Green
