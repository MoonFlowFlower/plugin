[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [string]$PackageRoot = "",
    [string]$ValidationRoot = "",
    [switch]$PackageFirst,
    [int]$EditorQuitTimeoutSeconds = 120,
    [int]$PostQuitGracePeriodSeconds = 10,
    [switch]$KeepValidationProject
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$FabRoot = Join-Path $ProjectRoot "Saved\FabRelease"
$PluginName = "RuntimeInspector"
$ValidationProjectName = "RIFabBlank"

if ([string]::IsNullOrWhiteSpace($PackageRoot)) {
    $PackageRoot = Join-Path $FabRoot "Package\RuntimeInspector_UE57\RuntimeInspector"
}
if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    $ValidationRoot = Join-Path $FabRoot "BlankHostLoadSmoke\RIFabBlank_UE57"
}
$PackageRoot = [System.IO.Path]::GetFullPath($PackageRoot)
$ValidationRoot = [System.IO.Path]::GetFullPath($ValidationRoot)

$ValidationProjectRoot = Join-Path $ValidationRoot $ValidationProjectName
$ValidationProjectFile = Join-Path $ValidationProjectRoot "$ValidationProjectName.uproject"
$ValidationPluginRoot = Join-Path $ValidationProjectRoot "Plugins\$PluginName"
$ValidationProjectLogPath = Join-Path $ValidationProjectRoot "Saved\Logs\$ValidationProjectName.log"
$ValidationLogPath = Join-Path $FabRoot "fab_blank_host_install_load_smoke_UE57.log"
$ValidationStdOutLogPath = Join-Path $FabRoot "fab_blank_host_install_load_smoke_UE57_stdout.log"
$ValidationStdErrLogPath = Join-Path $FabRoot "fab_blank_host_install_load_smoke_UE57_stderr.log"
$ValidationReportPath = Join-Path $FabRoot "Contracts\RuntimeInspector_UE57\blank-host-install-load-smoke.json"
$InputContractReportPath = Join-Path $FabRoot "Contracts\RuntimeInspector_UE57\blank-host-input-compiled-smoke.json"

function Reset-SafeDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$AllowedParent
    )

    $ResolvedPath = [System.IO.Path]::GetFullPath($Path).TrimEnd("\")
    $ResolvedParent = [System.IO.Path]::GetFullPath($AllowedParent).TrimEnd("\")
    if (-not $ResolvedPath.StartsWith(($ResolvedParent + "\"), [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to reset validation path outside Fab output root: $ResolvedPath"
    }
    if (Test-Path -LiteralPath $ResolvedPath) {
        Remove-Item -LiteralPath $ResolvedPath -Recurse -Force
    }
}

function Invoke-ArtifactContract {
    param(
        [Parameter(Mandatory = $true)][string]$Mode,
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Report
    )

    $ContractScript = Join-Path $ScriptRoot "TestFabArtifactContract.ps1"
    $PowerShellExe = Join-Path $env:SystemRoot "System32\WindowsPowerShell\v1.0\powershell.exe"
    $Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$ContractScript`" -Mode $Mode -PluginRoot `"$Root`" -ReportPath `"$Report`""
    $Process = Start-Process -FilePath $PowerShellExe -ArgumentList $Arguments -NoNewWindow -Wait -PassThru
    if ($Process.ExitCode -ne 0) {
        throw "Fab artifact contract failed ($Mode). Report: $Report"
    }
}

function Get-ValidationLifecycleLogSnapshot {
    $Chunks = @()
    foreach ($Path in @($ValidationProjectLogPath, $ValidationStdOutLogPath, $ValidationStdErrLogPath)) {
        if (Test-Path -LiteralPath $Path -PathType Leaf) {
            $Chunks += Get-Content -LiteralPath $Path -Raw
        }
    }
    return ($Chunks -join "`n")
}

function Append-ValidationLog {
    param([string]$SourcePath, [string]$DestinationPath)
    if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf)) {
        return
    }
    Add-Content -LiteralPath $DestinationPath -Value ("===== {0} =====" -f $SourcePath)
    Get-Content -LiteralPath $SourcePath | Add-Content -LiteralPath $DestinationPath
}

$BuildVersionPath = Join-Path $EngineRoot "Engine\Build\Build.version"
if (-not (Test-Path -LiteralPath $BuildVersionPath -PathType Leaf)) {
    throw "UE Build.version not found at: $BuildVersionPath"
}
$BuildVersion = Get-Content -LiteralPath $BuildVersionPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ([int]$BuildVersion.MajorVersion -ne 5 -or [int]$BuildVersion.MinorVersion -ne 7) {
    throw "Blank-host smoke requires UE 5.7; Build.version reports $($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion)."
}
$EngineAssociation = "{0}.{1}" -f [int]$BuildVersion.MajorVersion, [int]$BuildVersion.MinorVersion
$EngineVersion = "{0}.{1}.{2}" -f [int]$BuildVersion.MajorVersion, [int]$BuildVersion.MinorVersion, [int]$BuildVersion.PatchVersion

$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path -LiteralPath $EditorCmd -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe not found at: $EditorCmd"
}

if ($PackageFirst -or -not (Test-Path -LiteralPath (Join-Path $PackageRoot "$PluginName.uplugin") -PathType Leaf)) {
    & (Join-Path $ScriptRoot "PackageFabRelease.ps1") -EngineRoot $EngineRoot
}
if (-not (Test-Path -LiteralPath (Join-Path $PackageRoot "$PluginName.uplugin") -PathType Leaf)) {
    throw "Compiled-smoke plugin root not found: $PackageRoot"
}
if (-not (Test-Path -LiteralPath (Join-Path $PackageRoot "Binaries") -PathType Container)) {
    throw "Compiled-smoke plugin is missing Binaries: $PackageRoot"
}
Invoke-ArtifactContract -Mode "CompiledSmoke" -Root $PackageRoot -Report $InputContractReportPath

Reset-SafeDirectory -Path $ValidationRoot -AllowedParent $FabRoot
foreach ($Path in @($ValidationLogPath, $ValidationStdOutLogPath, $ValidationStdErrLogPath)) {
    Remove-Item -LiteralPath $Path -Force -ErrorAction SilentlyContinue
}

New-Item -ItemType Directory -Force -Path `
    $ValidationProjectRoot, `
    (Join-Path $ValidationProjectRoot "Config"), `
    (Join-Path $ValidationProjectRoot "Content"), `
    (Join-Path $ValidationProjectRoot "Plugins"), `
    (Split-Path -Parent $ValidationReportPath) | Out-Null

$UProject = [ordered]@{
    FileVersion = 3
    EngineAssociation = $EngineAssociation
    Category = ""
    Description = "Runtime Inspector Fab install/load smoke host."
    Plugins = @(
        [ordered]@{
            Name = $PluginName
            Enabled = $true
            SupportedTargetPlatforms = @("Win64")
        }
    )
}
$UProject | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ValidationProjectFile -Encoding UTF8

$DefaultEngineIni = @"
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Engine/Maps/Entry
EditorStartupMap=/Engine/Maps/Entry
"@
$DefaultGameIni = @"
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=9C0F94E2476C40E4B9166C1EC5221FB0
ProjectName=RIFabBlank
Description=Runtime Inspector Fab install/load smoke host.
"@
Set-Content -LiteralPath (Join-Path $ValidationProjectRoot "Config\DefaultEngine.ini") -Value $DefaultEngineIni -Encoding UTF8
Set-Content -LiteralPath (Join-Path $ValidationProjectRoot "Config\DefaultGame.ini") -Value $DefaultGameIni -Encoding UTF8

Copy-Item -LiteralPath $PackageRoot -Destination $ValidationPluginRoot -Recurse -Force
$HostContentEntryCountBeforeLaunch = @(Get-ChildItem -LiteralPath (Join-Path $ValidationProjectRoot "Content") -Force).Count

$EditorArgs = @(
    $ValidationProjectFile,
    "-NoSplash",
    "-NullRHI",
    "-Unattended",
    "-NoSound",
    "-stdout",
    "-FullStdOutLogOutput",
    "-ExecCmds=QUIT"
)
$EditorProcess = Start-Process `
    -FilePath $EditorCmd `
    -ArgumentList $EditorArgs `
    -NoNewWindow `
    -PassThru `
    -RedirectStandardOutput $ValidationStdOutLogPath `
    -RedirectStandardError $ValidationStdErrLogPath

$ForcedShutdownAfterQuit = $false
$TimedOut = $false
$QuitObservedAt = $null
$Deadline = (Get-Date).AddSeconds($EditorQuitTimeoutSeconds)
while ($true) {
    $EditorProcess.Refresh()
    if ($EditorProcess.HasExited) {
        break
    }

    $Snapshot = Get-ValidationLifecycleLogSnapshot
    $Mounted = $Snapshot -match "Mounting Project plugin RuntimeInspector"
    $QuitObserved = $Snapshot -match "Cmd:\s+QUIT"
    if ($Mounted -and $QuitObserved) {
        if ($null -eq $QuitObservedAt) {
            $QuitObservedAt = Get-Date
        }
        if ((Get-Date) -ge $QuitObservedAt.AddSeconds($PostQuitGracePeriodSeconds)) {
            Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
            $ForcedShutdownAfterQuit = $true
            break
        }
    } elseif ((Get-Date) -ge $Deadline) {
        Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
        $TimedOut = $true
        break
    }
    Start-Sleep -Milliseconds 500
}

Start-Sleep -Milliseconds 250
$EditorProcess.Refresh()
$ProcessExitCode = if ($EditorProcess.HasExited) { [int]$EditorProcess.ExitCode } else { -1 }

New-Item -ItemType File -Force -Path $ValidationLogPath | Out-Null
Append-ValidationLog -SourcePath $ValidationProjectLogPath -DestinationPath $ValidationLogPath
Append-ValidationLog -SourcePath $ValidationStdOutLogPath -DestinationPath $ValidationLogPath
Append-ValidationLog -SourcePath $ValidationStdErrLogPath -DestinationPath $ValidationLogPath
$ValidationLogContent = if (Test-Path -LiteralPath $ValidationLogPath -PathType Leaf) {
    Get-Content -LiteralPath $ValidationLogPath -Raw
} else {
    ""
}

$MountedPlugin = $ValidationLogContent -match "Mounting Project plugin RuntimeInspector"
$PluginIssuePatterns = @(
    "(?i)\[RI\]\[ToolsConfig\]",
    "(?i)(Error:|Warning:).*RuntimeInspector",
    "(?i)RuntimeInspector.*(failed to load|could not be found|unable to load|missing|parse failed|invalid)",
    "(?i)(ToolsSelfTestsDefault|ToolsWorkflowsDefault)\.json.*(missing|not found|parse|invalid|failed)",
    "(?i)(Error:|Warning:).*(ToolsSelfTestsDefault|ToolsWorkflowsDefault)\.json",
    "(?i)FConfigCacheIni::LoadFile failed loading file.*Filename was:\s+Game\b",
    "(?i)Project RIFabBlank requires update\. Plugin RuntimeInspector"
)
$PluginIssueLines = [System.Collections.Generic.List[string]]::new()
foreach ($Line in ($ValidationLogContent -split "`r?`n")) {
    foreach ($Pattern in $PluginIssuePatterns) {
        if ($Line -match $Pattern) {
            $PluginIssueLines.Add($Line.Trim())
            break
        }
    }
}
$PluginIssueLines = @($PluginIssueLines | Sort-Object -Unique)
$ExitAccepted = ($ProcessExitCode -eq 0) -or $ForcedShutdownAfterQuit
$Passed = (-not $TimedOut) -and $ExitAccepted -and $MountedPlugin -and $PluginIssueLines.Count -eq 0

$Report = [ordered]@{
    schema = "runtimeinspector.fab-blank-host-install-load-smoke.v1"
    generatedUtc = [DateTime]::UtcNow.ToString("o")
    passed = $Passed
    engineVersion = $EngineVersion
    engineAssociation = $EngineAssociation
    projectName = $ValidationProjectName
    projectFile = $ValidationProjectFile
    packageRoot = $PackageRoot
    installedPluginRoot = $ValidationPluginRoot
    hostContentDirectoryPresent = (Test-Path -LiteralPath (Join-Path $ValidationProjectRoot "Content") -PathType Container)
    hostContentEntryCountBeforeLaunch = $HostContentEntryCountBeforeLaunch
    hostContentEmptyBeforeLaunch = ($HostContentEntryCountBeforeLaunch -eq 0)
    defaultGameIniPresent = (Test-Path -LiteralPath (Join-Path $ValidationProjectRoot "Config\DefaultGame.ini") -PathType Leaf)
    mountedPlugin = $MountedPlugin
    processExitCode = $ProcessExitCode
    timedOut = $TimedOut
    forcedShutdownAfterQuit = $ForcedShutdownAfterQuit
    pluginIssueLines = $PluginIssueLines
    inputContractReport = $InputContractReportPath
    combinedLog = $ValidationLogPath
}
$Report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ValidationReportPath -Encoding UTF8

if (-not $Passed) {
    if ($PluginIssueLines.Count -gt 0) {
        Write-Host ($PluginIssueLines -join "`n") -ForegroundColor Red
    }
    throw "Fab blank-host install/load smoke failed. Report: $ValidationReportPath Log: $ValidationLogPath"
}

Write-Host ""
Write-Host "[PASS] Fab blank-host install/load smoke"
Write-Host "       Engine:  $EngineVersion (association $EngineAssociation)"
Write-Host "       Project: $ValidationProjectFile"
Write-Host "       Plugin:  $ValidationPluginRoot"
Write-Host "       Report:  $ValidationReportPath"
Write-Host "       Log:     $ValidationLogPath"
if ($ForcedShutdownAfterQuit) {
    Write-Host "       Note: QUIT was observed; editor was stopped after a $PostQuitGracePeriodSeconds-second grace period."
}

if (-not $KeepValidationProject) {
    Reset-SafeDirectory -Path $ValidationRoot -AllowedParent $FabRoot
    Write-Host "       Cleaned smoke host: $ValidationRoot"
}
