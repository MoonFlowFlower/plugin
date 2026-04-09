[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Software\Unreal\UE_5.5",
    [string]$PackageRoot = "",
    [string]$ValidationRoot = "",
    [switch]$PackageFirst,
    [int]$EditorQuitTimeoutSeconds = 120,
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

$TimedOutAfterQuit = $false
Wait-Process -Id $EditorProcess.Id -Timeout $EditorQuitTimeoutSeconds -ErrorAction SilentlyContinue

if (-not $EditorProcess.HasExited) {
    $ProjectLogSnapshot = if (Test-Path $ValidationProjectLogPath) {
        Get-Content -LiteralPath $ValidationProjectLogPath -Raw
    } else {
        ""
    }

    $MountedBeforeTimeout = $ProjectLogSnapshot -match "Mounting Project plugin RuntimeInspector"
    $ObservedQuitBeforeTimeout = $ProjectLogSnapshot -match "Cmd:\s+QUIT"

    if ($MountedBeforeTimeout -and $ObservedQuitBeforeTimeout) {
        Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
        $TimedOutAfterQuit = $true
    } else {
        Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
        throw "Blank project validation timed out before observing plugin mount + QUIT. See project log: $ValidationProjectLogPath"
    }
}

Wait-Process -Id $EditorProcess.Id -ErrorAction SilentlyContinue
$EditorProcess.Refresh()

New-Item -ItemType File -Path $ValidationLogPath -Force | Out-Null
if (Test-Path $ValidationProjectLogPath) {
    Get-Content -LiteralPath $ValidationProjectLogPath | Tee-Object -FilePath $ValidationLogPath -Append | Out-Host
}
if (Test-Path $ValidationStdOutLogPath) {
    Get-Content -LiteralPath $ValidationStdOutLogPath | Tee-Object -FilePath $ValidationLogPath -Append | Out-Host
}
if (Test-Path $ValidationStdErrLogPath) {
    Get-Content -LiteralPath $ValidationStdErrLogPath | Tee-Object -FilePath $ValidationLogPath -Append | Out-Host
}

if (($EditorProcess.ExitCode -ne 0) -and (-not $TimedOutAfterQuit)) {
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
if ($TimedOutAfterQuit) {
    Write-Warning "UnrealEditor-Cmd required forced shutdown after observing plugin mount + QUIT, but the package load validation still passed."
}

if (-not $KeepValidationProject) {
    Remove-Item -LiteralPath $ValidationRoot -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Validation project cleaned: $ValidationRoot"
}
