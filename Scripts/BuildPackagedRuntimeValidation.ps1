[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [string]$ValidationRoot = "",
    [switch]$ForceRebuild
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$ValidationRoot = if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    Join-Path $ProjectRoot "Saved\RuntimeInspector\PackagedRuntimeValidation"
} else {
    $ValidationRoot
}

$PackageRoot = Join-Path $ValidationRoot "Package"
$StatePath = Join-Path $ValidationRoot "state.json"
$GameBuildLogPath = Join-Path $ProjectRoot "Saved\build_packaged_runtime_validation_game.log"
$GameBuildStdOutLogPath = Join-Path $ProjectRoot "Saved\build_packaged_runtime_validation_game_stdout.log"
$GameBuildStdErrLogPath = Join-Path $ProjectRoot "Saved\build_packaged_runtime_validation_game_stderr.log"
$CookLogPath = Join-Path $ProjectRoot "Saved\build_packaged_runtime_validation.log"
$CookStdOutLogPath = Join-Path $ProjectRoot "Saved\build_packaged_runtime_validation_stdout.log"
$CookStdErrLogPath = Join-Path $ProjectRoot "Saved\build_packaged_runtime_validation_stderr.log"

$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$UnrealBuildToolDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
$AutomationToolDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll"
$BundledDotNetRoot = Join-Path $EngineRoot "Engine\Binaries\ThirdParty\DotNet"
$BundledDotNetExe = Join-Path $BundledDotNetRoot "8.0.300\win-x64\dotnet.exe"
if (-not (Test-Path $BundledDotNetExe -PathType Leaf)) {
    $BundledDotNetExe = Get-ChildItem -Path $BundledDotNetRoot -Filter dotnet.exe -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}

if (-not (Test-Path $ProjectFile -PathType Leaf)) {
    throw "Project file not found: $ProjectFile"
}
if (-not (Test-Path $RunUAT -PathType Leaf)) {
    throw "RunUAT.bat not found at: $RunUAT"
}
if (-not (Test-Path $UnrealBuildToolDll -PathType Leaf)) {
    throw "UnrealBuildTool.dll not found at: $UnrealBuildToolDll"
}
if (-not $BundledDotNetExe) {
    throw "Bundled dotnet runtime was not found under: $BundledDotNetRoot"
}
$BundledDotNetDir = Split-Path -Parent $BundledDotNetExe
$env:PATH = "$BundledDotNetDir;$env:PATH"

$ExpectedExePath = Join-Path $PackageRoot "Windows\PluginMaker.exe"
if ((-not $ForceRebuild) -and (Test-Path $ExpectedExePath -PathType Leaf)) {
    Write-Host "Packaged runtime validation build already exists."
    Write-Host "Package: $PackageRoot"
    Write-Host "Exe:     $ExpectedExePath"
    exit 0
}

Remove-Item -LiteralPath $PackageRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $StatePath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $GameBuildLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $GameBuildStdOutLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $GameBuildStdErrLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $CookLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $CookStdOutLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $CookStdErrLogPath -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $ValidationRoot -Force | Out-Null

$GameBuildProcess = Start-Process `
    -FilePath $BundledDotNetExe `
    -ArgumentList @(
        $UnrealBuildToolDll,
        "PluginMaker",
        "Win64",
        "Development",
        $ProjectFile,
        "-WaitMutex",
        "-NoHotReloadFromIDE",
        "-NoUBTMakefiles"
    ) `
    -NoNewWindow `
    -Wait `
    -PassThru `
    -RedirectStandardOutput $GameBuildStdOutLogPath `
    -RedirectStandardError $GameBuildStdErrLogPath

New-Item -ItemType File -Path $GameBuildLogPath -Force | Out-Null
if (Test-Path $GameBuildStdOutLogPath) {
    Get-Content -LiteralPath $GameBuildStdOutLogPath | Tee-Object -FilePath $GameBuildLogPath -Append | Out-Host
}
if (Test-Path $GameBuildStdErrLogPath) {
    Get-Content -LiteralPath $GameBuildStdErrLogPath | Tee-Object -FilePath $GameBuildLogPath -Append | Out-Host
}
if ($GameBuildProcess.ExitCode -ne 0) {
    throw "PluginMaker Win64 Development build failed. See log: $GameBuildLogPath"
}

$CookLaunchFile = $RunUAT
$CookArguments = @(
    "BuildCookRun",
    "-project=$ProjectFile",
    "-platform=Win64",
    "-clientconfig=Development",
    "-serverconfig=Development",
    "-build",
    "-nocompileeditor",
    "-skipbuildeditor",
    "-cook",
    "-stage",
    "-pak",
    "-archive",
    "-archivedirectory=$PackageRoot",
    "-nop4",
    "-utf8output"
)

if (Test-Path $AutomationToolDll -PathType Leaf) {
    $CookLaunchFile = $BundledDotNetExe
    $CookArguments = @(
        $AutomationToolDll,
        "BuildCookRun",
        "-project=$ProjectFile",
        "-platform=Win64",
        "-clientconfig=Development",
        "-serverconfig=Development",
        "-build",
        "-nocompileeditor",
        "-skipbuildeditor",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        "-archivedirectory=$PackageRoot",
        "-nop4",
        "-utf8output"
    )
}

$CookProcess = Start-Process `
    -FilePath $CookLaunchFile `
    -ArgumentList $CookArguments `
    -NoNewWindow `
    -Wait `
    -PassThru `
    -RedirectStandardOutput $CookStdOutLogPath `
    -RedirectStandardError $CookStdErrLogPath

New-Item -ItemType File -Path $CookLogPath -Force | Out-Null
if (Test-Path $CookStdOutLogPath) {
    Get-Content -LiteralPath $CookStdOutLogPath | Tee-Object -FilePath $CookLogPath -Append | Out-Host
}
if (Test-Path $CookStdErrLogPath) {
    Get-Content -LiteralPath $CookStdErrLogPath | Tee-Object -FilePath $CookLogPath -Append | Out-Host
}
if ($CookProcess.ExitCode -ne 0) {
    throw "BuildCookRun failed. See log: $CookLogPath"
}

if (-not (Test-Path $ExpectedExePath -PathType Leaf)) {
    throw "Packaged validation executable not found: $ExpectedExePath"
}

Write-Host ""
Write-Host "Packaged runtime validation build ready."
Write-Host "Package: $PackageRoot"
Write-Host "Exe:     $ExpectedExePath"
Write-Host "Logs:"
Write-Host "  Game:  $GameBuildLogPath"
Write-Host "  Cook:  $CookLogPath"
