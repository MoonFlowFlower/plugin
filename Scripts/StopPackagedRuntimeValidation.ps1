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
$PackageRoot = Join-Path $ValidationRoot "Package"
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

function Test-PathUnderRoot {
    param(
        [string]$Path,
        [string]$Root
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or [string]::IsNullOrWhiteSpace($Root)) {
        return $false
    }

    try {
        $ResolvedPath = [System.IO.Path]::GetFullPath($Path)
        $ResolvedRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
        return $ResolvedPath.StartsWith($ResolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)
    } catch {
        return $false
    }
}

function Get-PackagedRuntimeProcesses {
    $Processes = @()
    foreach ($Process in @(Get-Process -ErrorAction SilentlyContinue)) {
        try {
            if (Test-PathUnderRoot -Path ([string]$Process.Path) -Root $PackageRoot) {
                $Processes += $Process
            }
        } catch {
        }
    }

    return $Processes
}

function Stop-PackagedRuntimeProcesses {
    $Targets = @(Get-PackagedRuntimeProcesses | Sort-Object { ([string]$_.Path).Length } -Descending)
    foreach ($Target in $Targets) {
        try {
            Stop-Process -Id ([int]$Target.Id) -Force -ErrorAction Stop
        } catch {
        }
    }

    $Deadline = (Get-Date).AddSeconds(10)
    do {
        $Remaining = @(Get-PackagedRuntimeProcesses)
        if ($Remaining.Count -eq 0) {
            return @($Targets | ForEach-Object { [int]$_.Id })
        }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $Deadline)

    $RemainingSummary = ($Remaining | ForEach-Object { "$($_.Id):$($_.ProcessName):$($_.Path)" }) -join "; "
    throw "Could not stop all packaged validation processes under $PackageRoot. Remaining=$RemainingSummary"
}

Remove-Item -LiteralPath $StopLogPath -Force -ErrorAction SilentlyContinue

$State = Read-ValidationState
$StoppedProcessIds = @(Stop-PackagedRuntimeProcesses)

$StoppedState = [ordered]@{
    active = $false
    status = "stopped"
    project = "PluginMaker"
    stoppedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    previousPid = if ($State) { [int]$State.pid } else { 0 }
    previousPort = if ($State) { [int]$State.port } else { 0 }
    exePath = if ($State) { [string]$State.exePath } else { "" }
    stoppedProcessIds = $StoppedProcessIds
    remainingProcessCount = 0
}
Write-ValidationState $StoppedState

"Packaged runtime validation stopped. PID=$($StoppedState.previousPid) Port=$($StoppedState.previousPort) Processes=$($StoppedProcessIds -join ',')" | Tee-Object -FilePath $StopLogPath -Append | Out-Host
