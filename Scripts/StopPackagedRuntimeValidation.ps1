[CmdletBinding()]
param(
    [string]$ValidationRoot = ""
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$ValidationRoot = if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    Join-Path $ProjectRoot "Saved\RuntimeInspector\PackagedRuntimeValidation"
} else {
    $ValidationRoot
}
$StatePath = Join-Path $ValidationRoot "state.json"
$StopLogPath = Join-Path $ProjectRoot "Saved\stop_packaged_runtime_validation.log"

function Read-ValidationState {
    if (-not (Test-Path $StatePath -PathType Leaf)) {
        return $null
    }

    try {
        return Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Write-ValidationState {
    param([hashtable]$State)
    New-Item -ItemType Directory -Path (Split-Path -Parent $StatePath) -Force | Out-Null
    $State | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $StatePath -Encoding UTF8
}

Remove-Item -LiteralPath $StopLogPath -Force -ErrorAction SilentlyContinue

$State = Read-ValidationState
if ($State -and $State.pid) {
    try {
        Stop-Process -Id ([int]$State.pid) -Force -ErrorAction Stop
    } catch {
    }
}

$StoppedState = [ordered]@{
    active = $false
    status = "stopped"
    project = "PluginMaker"
    stoppedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    previousPid = if ($State) { [int]$State.pid } else { 0 }
    previousPort = if ($State) { [int]$State.port } else { 0 }
    exePath = if ($State) { [string]$State.exePath } else { "" }
}
Write-ValidationState $StoppedState

"Packaged runtime validation stopped. PID=$($StoppedState.previousPid) Port=$($StoppedState.previousPort)" | Tee-Object -FilePath $StopLogPath -Append | Out-Host
