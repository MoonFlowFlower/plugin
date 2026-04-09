[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Software\Unreal\UE_5.5",
    [string]$TargetPlatforms = "Win64",
    [string]$StageRoot = "",
    [string]$OutputRoot = "",
    [string]$ValidationRoot = "",
    [switch]$NoHostPlatform,
    [switch]$SkipProjectEditorBuild,
    [switch]$KeepValidationOutput,
    [switch]$KeepStage
)

$ErrorActionPreference = "Stop"
$UseNoHostPlatform = $true
if ($PSBoundParameters.ContainsKey("NoHostPlatform")) {
    $UseNoHostPlatform = [bool]$NoHostPlatform
}

function Copy-AllowedItem {
    param(
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$DestinationPath
    )

    if (Test-Path $SourcePath -PathType Container) {
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Recurse -Force
    } elseif (Test-Path $SourcePath -PathType Leaf) {
        $parent = Split-Path -Parent $DestinationPath
        if ($parent) {
            New-Item -ItemType Directory -Path $parent -Force | Out-Null
        }
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
    }
}

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($StageRoot)) {
    $StageRoot = Join-Path $ProjectRoot "Saved\FabRelease\Stage\RuntimeInspector"
}

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $ProjectRoot "Saved\FabRelease\Package\RuntimeInspector_UE55"
}

if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    $ValidationRoot = Join-Path $ProjectRoot "Saved\FabRelease\Validation\RuntimeInspector_UE55"
}

$LogPath = Join-Path $ProjectRoot "Saved\build_runtimeinspector_fab_release.log"
$StdOutLogPath = Join-Path $ProjectRoot "Saved\build_runtimeinspector_fab_release_stdout.log"
$StdErrLogPath = Join-Path $ProjectRoot "Saved\build_runtimeinspector_fab_release_stderr.log"
$EditorBuildLogPath = Join-Path $ProjectRoot "Saved\build_runtimeinspector_fab_editor_build.log"
$EditorBuildStdOutLogPath = Join-Path $ProjectRoot "Saved\build_runtimeinspector_fab_editor_build_stdout.log"
$EditorBuildStdErrLogPath = Join-Path $ProjectRoot "Saved\build_runtimeinspector_fab_editor_build_stderr.log"
$ProjectFile = Join-Path $ProjectRoot "PluginMaker.uproject"
$ProjectEditorTarget = "PluginMakerEditor"
$UnrealBuildToolDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
$LocalEditorBinarySourceRoot = Join-Path $PluginRoot "Binaries\Win64"
$StableEditorBinaryFiles = @(
    "UnrealEditor.modules",
    "UnrealEditor-RuntimeInspector.dll"
)
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$AutomationToolDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll"
$BundledDotNetRoot = Join-Path $EngineRoot "Engine\Binaries\ThirdParty\DotNet"

if (-not (Test-Path $RunUAT -PathType Leaf)) {
    throw "RunUAT.bat not found at: $RunUAT"
}

$BundledDotNetExe = Join-Path $BundledDotNetRoot "8.0.300\win-x64\dotnet.exe"
if (-not (Test-Path $BundledDotNetExe -PathType Leaf)) {
    $BundledDotNetExe = Get-ChildItem -Path $BundledDotNetRoot -Filter dotnet.exe -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}

if ($BundledDotNetExe) {
    $BundledDotNetDir = Split-Path -Parent $BundledDotNetExe
    $env:PATH = "$BundledDotNetDir;$env:PATH"
}

$AllowedDirectories = @(
    "Config",
    "Content",
    "Resources",
    "Source"
)

$PackagedAllowedDirectories = @(
    "Config",
    "Content",
    "Intermediate",
    "Resources",
    "Source"
)

$AllowedFiles = @(
    "RuntimeInspector.uplugin",
    "README.md",
    "USER_GUIDE_zh-CN.md"
)

$OptionalFiles = @(
    "LICENSE",
    "LICENSE.txt"
)

$ForbiddenEntries = @(
    ".git",
    "Binaries",
    "Intermediate",
    "Docs",
    "Saved"
)

$FinalForbiddenEntries = @(
    ".git",
    "Docs",
    "HostProject",
    "Saved"
)

Remove-Item -LiteralPath $StageRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $OutputRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $ValidationRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $EditorBuildLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $EditorBuildStdOutLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $EditorBuildStdErrLogPath -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $StageRoot -Force | Out-Null

foreach ($DirectoryName in $AllowedDirectories) {
    $SourcePath = Join-Path $PluginRoot $DirectoryName
    if (-not (Test-Path $SourcePath -PathType Container)) {
        throw "Missing required plugin directory: $SourcePath"
    }

    $DestinationPath = Join-Path $StageRoot $DirectoryName
    Copy-AllowedItem -SourcePath $SourcePath -DestinationPath $DestinationPath
}

foreach ($FileName in $AllowedFiles + $OptionalFiles) {
    $SourcePath = Join-Path $PluginRoot $FileName
    if (-not (Test-Path $SourcePath)) {
        if ($AllowedFiles -contains $FileName) {
            throw "Missing required plugin file: $SourcePath"
        }
        continue
    }

    $DestinationPath = Join-Path $StageRoot $FileName
    Copy-AllowedItem -SourcePath $SourcePath -DestinationPath $DestinationPath
}

foreach ($ForbiddenEntry in $ForbiddenEntries) {
    $ForbiddenPath = Join-Path $StageRoot $ForbiddenEntry
    if (Test-Path $ForbiddenPath) {
        throw "Forbidden entry leaked into staged plugin: $ForbiddenPath"
    }
}

$StagedPlugin = Join-Path $StageRoot "RuntimeInspector.uplugin"

if (-not $SkipProjectEditorBuild) {
    if (-not (Test-Path $ProjectFile -PathType Leaf)) {
        throw "Project file not found for editor binary build: $ProjectFile"
    }
    if (-not (Test-Path $UnrealBuildToolDll -PathType Leaf)) {
        throw "UnrealBuildTool.dll not found at: $UnrealBuildToolDll"
    }
    if (-not $BundledDotNetExe) {
        throw "Bundled dotnet runtime was not found under: $BundledDotNetRoot"
    }

    $EditorBuildProcess = Start-Process `
        -FilePath $BundledDotNetExe `
        -ArgumentList @(
            $UnrealBuildToolDll,
            $ProjectEditorTarget,
            "Win64",
            "Development",
            "-Project=$ProjectFile",
            "-WaitMutex",
            "-NoHotReloadFromIDE",
            "-NoUBTMakefiles"
        ) `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $EditorBuildStdOutLogPath `
        -RedirectStandardError $EditorBuildStdErrLogPath

    New-Item -ItemType File -Path $EditorBuildLogPath -Force | Out-Null
    if (Test-Path $EditorBuildStdOutLogPath) {
        Get-Content -LiteralPath $EditorBuildStdOutLogPath | Tee-Object -FilePath $EditorBuildLogPath -Append | Out-Host
    }
    if (Test-Path $EditorBuildStdErrLogPath) {
        Get-Content -LiteralPath $EditorBuildStdErrLogPath | Tee-Object -FilePath $EditorBuildLogPath -Append | Out-Host
    }

    if ($EditorBuildProcess.ExitCode -ne 0) {
        throw "Project editor build failed. See log: $EditorBuildLogPath"
    }
}

foreach ($BinaryFileName in $StableEditorBinaryFiles) {
    $BinaryPath = Join-Path $LocalEditorBinarySourceRoot $BinaryFileName
    if (-not (Test-Path $BinaryPath -PathType Leaf)) {
        throw "Stable editor binary is missing after build: $BinaryPath"
    }
}

Remove-Item -LiteralPath $LogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $StdOutLogPath -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $StdErrLogPath -Force -ErrorAction SilentlyContinue

$LaunchFilePath = $RunUAT
$LaunchArguments = @(
    "BuildPlugin",
    "-Plugin=$StagedPlugin",
    "-Package=$ValidationRoot",
    "-TargetPlatforms=$TargetPlatforms"
)

if ($UseNoHostPlatform) {
    $LaunchArguments += "-NoHostPlatform"
}

if ($BundledDotNetExe -and (Test-Path $AutomationToolDll -PathType Leaf)) {
    $LaunchFilePath = $BundledDotNetExe
    $LaunchArguments = @(
        $AutomationToolDll,
        "BuildPlugin",
        "-Plugin=$StagedPlugin",
        "-Package=$ValidationRoot",
        "-TargetPlatforms=$TargetPlatforms"
    )

    if ($UseNoHostPlatform) {
        $LaunchArguments += "-NoHostPlatform"
    }
}

$UatProcess = Start-Process `
    -FilePath $LaunchFilePath `
    -ArgumentList $LaunchArguments `
    -NoNewWindow `
    -Wait `
    -PassThru `
    -RedirectStandardOutput $StdOutLogPath `
    -RedirectStandardError $StdErrLogPath

New-Item -ItemType File -Path $LogPath -Force | Out-Null
if (Test-Path $StdOutLogPath) {
    Get-Content -LiteralPath $StdOutLogPath | Tee-Object -FilePath $LogPath -Append | Out-Host
}
if (Test-Path $StdErrLogPath) {
    Get-Content -LiteralPath $StdErrLogPath | Tee-Object -FilePath $LogPath -Append | Out-Host
}

if ($UatProcess.ExitCode -ne 0) {
    throw "BuildPlugin failed. See log: $LogPath"
}

$ValidationPluginRootCandidates = @(
    $ValidationRoot,
    (Join-Path $ValidationRoot "RuntimeInspector"),
    (Join-Path $ValidationRoot "HostProject\Plugins\RuntimeInspector")
)

$ValidationPluginRoot = $null
foreach ($Candidate in $ValidationPluginRootCandidates) {
    if (Test-Path (Join-Path $Candidate "RuntimeInspector.uplugin")) {
        $ValidationPluginRoot = $Candidate
        break
    }
}

if (-not $ValidationPluginRoot) {
    throw "Unable to locate validated RuntimeInspector.uplugin under $ValidationRoot"
}

$LeakedValidationDirectories = Get-ChildItem -LiteralPath $ValidationPluginRoot -Directory -Force |
    Where-Object { $ForbiddenEntries -contains $_.Name } |
    Select-Object -ExpandProperty FullName

if (-not $KeepValidationOutput -and $LeakedValidationDirectories) {
    Write-Warning ("BuildPlugin validation output contains generated directories and will not be shipped:`n" + ($LeakedValidationDirectories -join "`n"))
}

$FinalPluginRoot = Join-Path $OutputRoot "RuntimeInspector"
New-Item -ItemType Directory -Path $FinalPluginRoot -Force | Out-Null

foreach ($DirectoryName in $PackagedAllowedDirectories) {
    $SourcePath = Join-Path $ValidationPluginRoot $DirectoryName
    if (-not (Test-Path $SourcePath -PathType Container)) {
        continue
    }

    $DestinationPath = Join-Path $FinalPluginRoot $DirectoryName
    Copy-AllowedItem -SourcePath $SourcePath -DestinationPath $DestinationPath
}

foreach ($FileName in $AllowedFiles + $OptionalFiles) {
    $SourcePath = Join-Path $ValidationPluginRoot $FileName
    if (-not (Test-Path $SourcePath)) {
        continue
    }

    $DestinationPath = Join-Path $FinalPluginRoot $FileName
    Copy-AllowedItem -SourcePath $SourcePath -DestinationPath $DestinationPath
}

$FinalBinaryRoot = Join-Path $FinalPluginRoot "Binaries\Win64"
New-Item -ItemType Directory -Path $FinalBinaryRoot -Force | Out-Null
foreach ($BinaryFileName in $StableEditorBinaryFiles) {
    $SourcePath = Join-Path $LocalEditorBinarySourceRoot $BinaryFileName
    $DestinationPath = Join-Path $FinalBinaryRoot $BinaryFileName
    Copy-AllowedItem -SourcePath $SourcePath -DestinationPath $DestinationPath
}

$LeakedFinalDirectories = Get-ChildItem -LiteralPath $FinalPluginRoot -Directory -Force |
    Where-Object { $FinalForbiddenEntries -contains $_.Name } |
    Select-Object -ExpandProperty FullName

if ($LeakedFinalDirectories) {
    throw ("Forbidden directories present in final Fab package:`n" + ($LeakedFinalDirectories -join "`n"))
}

if (-not (Test-Path (Join-Path $FinalPluginRoot "README.md") -PathType Leaf)) {
    throw "Final Fab package is missing README.md"
}

if (-not (Test-Path (Join-Path $FinalPluginRoot "Binaries") -PathType Container)) {
    throw "Final Fab package is missing precompiled Binaries output."
}

if (-not (Test-Path (Join-Path $FinalPluginRoot "Intermediate") -PathType Container)) {
    Write-Warning "Final Fab package does not contain Intermediate precompile data."
}

Write-Host ""
Write-Host "Fab release package ready."
Write-Host "Stage:   $StageRoot"
Write-Host "Validate: $ValidationPluginRoot"
Write-Host "Package:  $FinalPluginRoot"
Write-Host "Log:     $LogPath"

if (-not $KeepStage) {
    Remove-Item -LiteralPath $StageRoot -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Stage cleaned: $StageRoot"
}

if (-not $KeepValidationOutput) {
    Remove-Item -LiteralPath $ValidationRoot -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "Validation cleaned: $ValidationRoot"
}
