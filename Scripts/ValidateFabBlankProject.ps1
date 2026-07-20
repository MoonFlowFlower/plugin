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

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($PackageRoot)) {
    $PackageRoot = Join-Path $ProjectRoot "Saved\FabRelease\Package\RuntimeInspector_UE55\RuntimeInspector"
}

if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    $ValidationRoot = Join-Path $ProjectRoot "Saved\FabRelease\BlankProjectValidation\RuntimeInspectorBlank_UE55"
}

$ValidationProjectRoot = Join-Path $ValidationRoot "RuntimeInspectorBlank"
$ValidationProjectFile = Join-Path $ValidationProjectRoot "RuntimeInspectorBlank.uproject"
$ValidationPluginRoot = Join-Path $ValidationProjectRoot "Plugins\RuntimeInspector"
$ValidationProjectLogPath = Join-Path $ValidationProjectRoot "Saved\Logs\RuntimeInspectorBlank.log"
$ValidationLogPath = Join-Path $ProjectRoot "Saved\fab_blank_project_validation.log"
$ValidationStdOutLogPath = Join-Path $ProjectRoot "Saved\fab_blank_project_validation_stdout.log"
$ValidationStdErrLogPath = Join-Path $ProjectRoot "Saved\fab_blank_project_validation_stderr.log"

function Get-ValidationLifecycleLogSnapshot {
    $LogChunks = @()
    foreach ($Path in @($ValidationProjectLogPath, $ValidationStdOutLogPath, $ValidationStdErrLogPath)) {
        if (Test-Path $Path) {
            $LogChunks += Get-Content -LiteralPath $Path -Raw
        }
    }

    return ($LogChunks -join "`n")
}

$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $EditorCmd -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe not found at: $EditorCmd"
}

$PackageScript = Join-Path $ScriptRoot "PackageFabRelease.ps1"
if ($PackageFirst -or -not (Test-Path (Join-Path $PackageRoot "RuntimeInspector.uplugin") -PathType Leaf)) {
    & $PackageScript -EngineRoot $EngineRoot
}

if (-not (Test-Path (Join-Path $PackageRoot "RuntimeInspector.uplugin") -PathType Leaf)) {
    throw "Packaged plugin root not found: $PackageRoot"
}

if (-not (Test-Path (Join-Path $PackageRoot "Binaries") -PathType Container)) {
    throw "Packaged plugin is missing precompiled Binaries: $PackageRoot"
}

Remove-Item -LiteralPath $ValidationRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $ValidationLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $ValidationStdOutLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $ValidationStdErrLogPath -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Path $ValidationProjectRoot -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $ValidationProjectRoot "Config") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $ValidationProjectRoot "Plugins") -Force | Out-Null

$UProjectJson = @"
{
  "FileVersion": 3,
  "EngineAssociation": "5.5",
  "Category": "",
  "Description": "Runtime Inspector Fab blank-project validation host.",
  "Plugins": [
    {
      "Name": "RuntimeInspector",
      "Enabled": true
    }
  ]
}
"@
Set-Content -LiteralPath $ValidationProjectFile -Value $UProjectJson -Encoding UTF8

$DefaultEngineIni = @"
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Engine/Maps/Entry
EditorStartupMap=/Engine/Maps/Entry
"@
Set-Content -LiteralPath (Join-Path $ValidationProjectRoot "Config\DefaultEngine.ini") -Value $DefaultEngineIni -Encoding UTF8

Copy-Item -LiteralPath $PackageRoot -Destination $ValidationPluginRoot -Recurse -Force

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
$QuitObservedAt = $null
$Deadline = (Get-Date).AddSeconds($EditorQuitTimeoutSeconds)
while ($true) {
    $EditorProcess.Refresh()
    if ($EditorProcess.HasExited) {
        break
    }

    $ProjectLogSnapshot = Get-ValidationLifecycleLogSnapshot

    $MountedBeforeTimeout = $ProjectLogSnapshot -match "Mounting Project plugin RuntimeInspector"
    $ObservedQuitBeforeTimeout = $ProjectLogSnapshot -match "Cmd:\s+QUIT"

    if ($MountedBeforeTimeout -and $ObservedQuitBeforeTimeout) {
        if ($null -eq $QuitObservedAt) {
            $QuitObservedAt = Get-Date
        }

        if ((Get-Date) -ge $QuitObservedAt.AddSeconds($PostQuitGracePeriodSeconds)) {
            Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
            $ForcedShutdownAfterQuit = $true
            break
        }

        Start-Sleep -Milliseconds 500
        continue
    }

    if ((Get-Date) -ge $Deadline) {
        $ProjectLogSnapshot = Get-ValidationLifecycleLogSnapshot

        $MountedBeforeTimeout = $ProjectLogSnapshot -match "Mounting Project plugin RuntimeInspector"
        $ObservedQuitBeforeTimeout = $ProjectLogSnapshot -match "Cmd:\s+QUIT"

        if ($MountedBeforeTimeout -and $ObservedQuitBeforeTimeout) {
            Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
            $ForcedShutdownAfterQuit = $true
            break
        }

        Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
        throw "Blank project validation timed out before observing plugin mount + QUIT. See log: $ValidationProjectLogPath"
    }

    Start-Sleep -Milliseconds 500
}

Start-Sleep -Milliseconds 200
$EditorProcess.Refresh()

function Append-ValidationLog {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    if (-not (Test-Path $SourcePath)) {
        return
    }

    Add-Content -LiteralPath $DestinationPath -Value ("===== {0} =====" -f $SourcePath)
    Get-Content -LiteralPath $SourcePath | Add-Content -LiteralPath $DestinationPath
}

New-Item -ItemType File -Path $ValidationLogPath -Force | Out-Null
Append-ValidationLog -SourcePath $ValidationProjectLogPath -DestinationPath $ValidationLogPath
Append-ValidationLog -SourcePath $ValidationStdOutLogPath -DestinationPath $ValidationLogPath
Append-ValidationLog -SourcePath $ValidationStdErrLogPath -DestinationPath $ValidationLogPath

if (($EditorProcess.ExitCode -ne 0) -and (-not $ForcedShutdownAfterQuit)) {
    throw "Blank project validation failed. See log: $ValidationLogPath"
}

$ValidationLogContent = if (Test-Path $ValidationLogPath) {
    Get-Content -LiteralPath $ValidationLogPath -Raw
} else {
    ""
}

$MountedPlugin = $ValidationLogContent -match "Mounting Project plugin RuntimeInspector"
$ObservedPluginLoadFailure = $false
foreach ($Pattern in @(
    "Plugin 'RuntimeInspector' failed to load",
    "module 'RuntimeInspector' could not be found",
    "Unable to load plugin 'RuntimeInspector'",
    "Failed to load.*RuntimeInspector",
    "Missing.*RuntimeInspector"
)) {
    if ($ValidationLogContent -match $Pattern) {
        $ObservedPluginLoadFailure = $true
        break
    }
}

if (-not $MountedPlugin) {
    throw "Blank project validation did not detect RuntimeInspector plugin mount. See log: $ValidationLogPath"
}

if ($ObservedPluginLoadFailure) {
    throw "Blank project validation log contains RuntimeInspector load errors. Confirm the packaged plugin includes precompiled Binaries. See log: $ValidationLogPath"
}

Write-Host ""
Write-Host "Blank project validation passed."
Write-Host "Project: $ValidationProjectFile"
Write-Host "Plugin:  $ValidationPluginRoot"
Write-Host "Log:     $ValidationLogPath"
if ($ForcedShutdownAfterQuit) {
    Write-Host "Editor stayed alive after QUIT; performed controlled shutdown after $PostQuitGracePeriodSeconds second grace period."
}

if (-not $KeepValidationProject) {
    Remove-Item -LiteralPath $ValidationRoot -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Validation project cleaned: $ValidationRoot"
}
