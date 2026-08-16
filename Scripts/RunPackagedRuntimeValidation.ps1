[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [string]$ValidationRoot = "",
    [switch]$ForceRestart,
    [switch]$BuildIfMissing
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
$ExePath = Join-Path $PackageRoot "Windows\PluginMaker.exe"
$StatePath = Join-Path $ValidationRoot "state.json"
$PackagedBridgeStatePath = Join-Path $PackageRoot "Windows\PluginMaker\Saved\UE_MCP_Bridge\bridge_state_runtime.json"
$RunLogPath = Join-Path $ProjectRoot "Saved\run_packaged_runtime_validation.log"
$PortCandidates = 12097..12101
$script:LastPortResolutionError = ""

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

function Test-ValidationProcess {
    param([object]$State)

    if (-not $State) {
        return $false
    }
    if (-not $State.active) {
        return $false
    }
    if (-not $State.pid) {
        return $false
    }

    try {
        $Process = Get-Process -Id ([int]$State.pid) -ErrorAction Stop
    } catch {
        return $false
    }

    if ($State.exePath -and (Test-Path -LiteralPath $State.exePath -PathType Leaf)) {
        $ExpectedExePath = [System.IO.Path]::GetFullPath([string]$State.exePath)
        try {
            if ($Process.Path -and ([System.IO.Path]::GetFullPath($Process.Path) -ne $ExpectedExePath)) {
                return $false
            }
        } catch {
        }
    }

    if (-not $State.port) {
        return $false
    }

    $ExpectedPackageRoot = if ($State.packageRoot) { [string]$State.packageRoot } else { $PackageRoot }
    return (Test-PackagedRuntimePortOwnership -Port ([int]$State.port) -ExpectedPackageRoot $ExpectedPackageRoot)
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

function Get-ListeningPortOwners {
    param([int]$Port)

    $Owners = @()
    $Connections = @(Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
    foreach ($Connection in $Connections) {
        $ProcessPath = ""
        $ProcessName = ""
        try {
            $OwnerProcess = Get-Process -Id ([int]$Connection.OwningProcess) -ErrorAction Stop
            $ProcessPath = [string]$OwnerProcess.Path
            $ProcessName = [string]$OwnerProcess.ProcessName
        } catch {
        }

        $Owners += [pscustomobject]@{
            pid = [int]$Connection.OwningProcess
            processName = $ProcessName
            processPath = $ProcessPath
        }
    }

    return $Owners
}

function Test-PackagedRuntimePortOwnership {
    param(
        [int]$Port,
        [string]$ExpectedPackageRoot
    )

    if (-not (Test-TcpPort -Port $Port)) {
        $script:LastPortResolutionError = "Port $Port is not accepting TCP connections."
        return $false
    }

    $Owners = @(Get-ListeningPortOwners -Port $Port)
    if ($Owners.Count -eq 0) {
        $script:LastPortResolutionError = "Port $Port has no queryable listening owner."
        return $false
    }

    $PackagedOwners = @($Owners | Where-Object { Test-PathUnderRoot -Path $_.processPath -Root $ExpectedPackageRoot })
    $ForeignOwners = @($Owners | Where-Object { -not (Test-PathUnderRoot -Path $_.processPath -Root $ExpectedPackageRoot) })
    if ($PackagedOwners.Count -eq 0) {
        $OwnerSummary = ($Owners | ForEach-Object { "$($_.pid):$($_.processName):$($_.processPath)" }) -join "; "
        $script:LastPortResolutionError = "Port $Port is not owned by the packaged runtime. Owners=$OwnerSummary"
        return $false
    }
    if ($ForeignOwners.Count -gt 0) {
        $OwnerSummary = ($Owners | ForEach-Object { "$($_.pid):$($_.processName):$($_.processPath)" }) -join "; "
        $script:LastPortResolutionError = "Port $Port has conflicting listeners. Owners=$OwnerSummary"
        return $false
    }

    $script:LastPortResolutionError = ""
    return $true
}

function Resolve-PackagedListeningPort {
    if (-not (Test-Path $PackagedBridgeStatePath -PathType Leaf)) {
        $script:LastPortResolutionError = "Packaged bridge state has not been written yet: $PackagedBridgeStatePath"
        return $null
    }

    try {
        $BridgeState = Get-Content -LiteralPath $PackagedBridgeStatePath -Raw | ConvertFrom-Json
    } catch {
        $script:LastPortResolutionError = "Packaged bridge state is not valid JSON: $PackagedBridgeStatePath"
        return $null
    }

    if (-not $BridgeState.active -or [string]$BridgeState.status -ne "running") {
        $script:LastPortResolutionError = "Packaged bridge state is not running."
        return $null
    }

    $Port = [int]$BridgeState.port
    if ($Port -notin $PortCandidates) {
        $script:LastPortResolutionError = "Packaged bridge reported port $Port outside $($PortCandidates[0])-$($PortCandidates[-1])."
        return $null
    }

    if (Test-PackagedRuntimePortOwnership -Port $Port -ExpectedPackageRoot $PackageRoot) {
        return $Port
    }

    return $null
}

function Ensure-PackagedBuild {
    if ((Test-Path $ExePath -PathType Leaf) -and -not $BuildIfMissing) {
        return
    }

    $BuildScript = Join-Path $ScriptRoot "BuildPackagedRuntimeValidation.ps1"
    if ($BuildIfMissing) {
        & $BuildScript -EngineRoot $EngineRoot -ForceRebuild
    } else {
        & $BuildScript -EngineRoot $EngineRoot
    }
}

Remove-Item -LiteralPath $RunLogPath -Force -ErrorAction SilentlyContinue

$CurrentState = Read-ValidationState
if (-not $ForceRestart) {
    if (Test-ValidationProcess $CurrentState) {
        $Port = [int]$CurrentState.port
        $ReadyState = [ordered]@{
            active = $true
            status = "reused"
            project = "PluginMaker"
            buildConfiguration = "Development"
            authority = "UE_MCP_Bridge_Runtime"
            host = "127.0.0.1"
            port = $Port
            pid = [int]$CurrentState.pid
            exePath = [string]$CurrentState.exePath
            packageRoot = [string]$CurrentState.packageRoot
            startedAtUtc = [string]$CurrentState.startedAtUtc
            resolvedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
        }
        Write-ValidationState $ReadyState
        "Packaged runtime validation reused. PID=$($ReadyState.pid) Port=$Port" | Tee-Object -FilePath $RunLogPath -Append | Out-Host
        exit 0
    }
}

# The launcher executable creates a child game process. Clean the entire
# dedicated package tree, not only the launcher PID recorded in state.json,
# so an orphan cannot retain a bridge port and contaminate the next run.
$StoppedProcessIds = @(Stop-PackagedRuntimeProcesses)

Ensure-PackagedBuild

if (-not (Test-Path $ExePath -PathType Leaf)) {
    throw "Packaged validation executable not found: $ExePath"
}

# The packaged bridge owns this file. Remove stale state before launch so an
# earlier run cannot make an unrelated editor/PIE listener look ready.
Remove-Item -LiteralPath $PackagedBridgeStatePath -Force -ErrorAction SilentlyContinue

$Process = Start-Process `
    -FilePath $ExePath `
    -WorkingDirectory (Split-Path -Parent $ExePath) `
    -ArgumentList @("-windowed", "-ResX=1600", "-ResY=900", "-log") `
    -PassThru

$ReadyPort = $null
$Deadline = (Get-Date).AddSeconds(120)
while ((Get-Date) -lt $Deadline) {
    $Process.Refresh()
    if ($Process.HasExited) {
        throw "Packaged validation process exited early. PID=$($Process.Id)"
    }

    $ReadyPort = Resolve-PackagedListeningPort
    if ($ReadyPort) {
        break
    }

    Start-Sleep -Seconds 1
}

if (-not $ReadyPort) {
    try {
        Stop-Process -Id $Process.Id -Force -ErrorAction Stop
    } catch {
    }
    throw "Packaged validation runtime bridge did not acquire a unique listener on ports 12097-12101. Last check: $script:LastPortResolutionError"
}

$ReadyState = [ordered]@{
    active = $true
    status = "ready"
    project = "PluginMaker"
    buildConfiguration = "Development"
    authority = "UE_MCP_Bridge_Runtime"
    host = "127.0.0.1"
    port = [int]$ReadyPort
    pid = $Process.Id
    exePath = $ExePath
    packageRoot = (Split-Path -Parent $ExePath)
    startedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
}
Write-ValidationState $ReadyState

"Packaged runtime validation ready. PID=$($Process.Id) Port=$ReadyPort Exe=$ExePath" | Tee-Object -FilePath $RunLogPath -Append | Out-Host
