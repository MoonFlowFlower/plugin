[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [string]$ValidationRoot = ""
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    $ValidationRoot = Join-Path $ProjectRoot "Saved\FabRelease\BlankHostLoadSmoke\RIFabBlank_UE57"
}

$ValidationProjectFile = Join-Path $ValidationRoot "RIFabBlank\RIFabBlank.uproject"
$EditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path $ValidationProjectFile -PathType Leaf)) {
    throw "Validation project not found. Run ValidateFabBlankProject.ps1 -KeepValidationProject first."
}

if (-not (Test-Path $EditorExe -PathType Leaf)) {
    throw "UnrealEditor.exe not found at: $EditorExe"
}

Start-Process -FilePath $EditorExe -ArgumentList $ValidationProjectFile
Write-Host "Opened validation project: $ValidationProjectFile"
