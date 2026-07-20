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
$RunLogPath = Join-Path $ProjectRoot "Saved\run_packaged_runtime_validation.log"
$PortCandidates = 12097..12101

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

    return (Test-TcpPort -Port ([int]$State.port))
}

function Stop-StaleValidationProcess {
    param([object]$State)

    if (-not $State -or -not $State.pid) {
        return
    }

    try {
        Stop-Process -Id ([int]$State.pid) -Force -ErrorAction Stop
    } catch {
    }
}

function Resolve-ListeningPort {
    foreach ($Port in $PortCandidates) {
        if (Test-TcpPort -Port $Port) {
            return $Port
        }
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

if ($CurrentState) {
    Stop-StaleValidationProcess $CurrentState
}

Ensure-PackagedBuild

if (-not (Test-Path $ExePath -PathType Leaf)) {
    throw "Packaged validation executable not found: $ExePath"
}

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

    $ReadyPort = Resolve-ListeningPort
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
    throw "Packaged validation runtime bridge did not begin listening on ports 12097-12101."
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
