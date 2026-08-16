[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [string]$TargetPlatforms = "Win64",
    [string]$SourceZipPath = "",
    [string]$SourceManifestPath = "",
    [string]$StageRoot = "",
    [string]$OutputRoot = "",
    [string]$ValidationRoot = "",
    [string]$SourceContractReportPath = "",
    [string]$CompiledContractReportPath = "",
    [ValidateRange(1, 64)]
    [int]$MaxParallelActions = 4,
    [switch]$NoHostPlatform,
    [switch]$SkipProjectEditorBuild,
    [switch]$KeepValidationOutput,
    [switch]$KeepStage
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$ProjectRoot = (Resolve-Path (Join-Path $PluginRoot "..\..")).Path
$PluginName = "RuntimeInspector"
$FabRoot = Join-Path $ProjectRoot "Saved\FabRelease"

if ([string]::IsNullOrWhiteSpace($SourceZipPath)) {
    $MakeSourceArtifact = Join-Path $ScriptRoot "MakeFabSubmissionZip.ps1"
    & $MakeSourceArtifact -EngineRoot $EngineRoot
    $SourceZipPath = Join-Path $PluginRoot "Saved\FabRelease\Submission\$PluginName-Fab-Submission.zip"
}
$SourceZipPath = [System.IO.Path]::GetFullPath($SourceZipPath)

if ([string]::IsNullOrWhiteSpace($SourceManifestPath)) {
    $SourceManifestPath = [System.IO.Path]::ChangeExtension($SourceZipPath, "manifest.json")
}
$SourceManifestPath = [System.IO.Path]::GetFullPath($SourceManifestPath)

if ([string]::IsNullOrWhiteSpace($StageRoot)) {
    $StageRoot = Join-Path $FabRoot "Stage\RuntimeInspector_UE57"
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $FabRoot "Package\RuntimeInspector_UE57"
}
if ([string]::IsNullOrWhiteSpace($ValidationRoot)) {
    $ValidationRoot = Join-Path $FabRoot "Validation\RuntimeInspector_UE57"
}
$ContractRoot = Join-Path $FabRoot "Contracts\RuntimeInspector_UE57"
if ([string]::IsNullOrWhiteSpace($SourceContractReportPath)) {
    $SourceContractReportPath = Join-Path $ContractRoot "source-submission.json"
}
if ([string]::IsNullOrWhiteSpace($CompiledContractReportPath)) {
    $CompiledContractReportPath = Join-Path $ContractRoot "compiled-smoke.json"
}

$StageRoot = [System.IO.Path]::GetFullPath($StageRoot)
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
$ValidationRoot = [System.IO.Path]::GetFullPath($ValidationRoot)
$SourceContractReportPath = [System.IO.Path]::GetFullPath($SourceContractReportPath)
$CompiledContractReportPath = [System.IO.Path]::GetFullPath($CompiledContractReportPath)

$LogPath = Join-Path $FabRoot "build_runtimeinspector_fab_release_UE57.log"
$StdOutLogPath = Join-Path $FabRoot "build_runtimeinspector_fab_release_UE57_stdout.log"
$StdErrLogPath = Join-Path $FabRoot "build_runtimeinspector_fab_release_UE57_stderr.log"
$ProvenancePath = Join-Path $ContractRoot "compiled-smoke-provenance.json"

function Convert-BytesToHex {
    param([byte[]]$Bytes)
    return ([System.BitConverter]::ToString($Bytes)).Replace("-", "").ToLowerInvariant()
}

function Get-FileSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    $Stream = [System.IO.File]::OpenRead($Path)
    try {
        $Hasher = [System.Security.Cryptography.SHA256]::Create()
        try {
            return Convert-BytesToHex -Bytes ($Hasher.ComputeHash($Stream))
        } finally {
            $Hasher.Dispose()
        }
    } finally {
        $Stream.Dispose()
    }
}

function Get-ZipPayloadInfo {
    param([Parameter(Mandatory = $true)][string]$Path)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $Archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $Files = @($Archive.Entries | Where-Object { -not [string]::IsNullOrEmpty($_.Name) } | Sort-Object FullName)
        $TopLevels = @($Files | ForEach-Object { ($_.FullName -split "/", 2)[0] } | Sort-Object -Unique)
        if ($TopLevels.Count -ne 1 -or $TopLevels[0] -ne $PluginName) {
            throw "Source ZIP must contain exactly one '$PluginName/' top-level directory. Found: $($TopLevels -join ', ')"
        }

        $Rows = [System.Collections.Generic.List[string]]::new()
        foreach ($Entry in $Files) {
            if ($Entry.FullName.Contains("\") -or $Entry.FullName.StartsWith("/") -or $Entry.FullName -match "(^|/)\.\.(/|$)") {
                throw "Unsafe ZIP entry: $($Entry.FullName)"
            }
            $Stream = $Entry.Open()
            try {
                $Hasher = [System.Security.Cryptography.SHA256]::Create()
                try {
                    $Hash = Convert-BytesToHex -Bytes ($Hasher.ComputeHash($Stream))
                } finally {
                    $Hasher.Dispose()
                }
            } finally {
                $Stream.Dispose()
            }
            $Rows.Add("$($Entry.FullName)`t$Hash")
        }

        $Material = [System.Text.Encoding]::UTF8.GetBytes((($Rows -join "`n") + "`n"))
        $TreeHasher = [System.Security.Cryptography.SHA256]::Create()
        try {
            $TreeHash = Convert-BytesToHex -Bytes ($TreeHasher.ComputeHash($Material))
        } finally {
            $TreeHasher.Dispose()
        }
        return [pscustomobject]@{
            entryCount = $Files.Count
            payloadTreeSha256 = $TreeHash
        }
    } finally {
        $Archive.Dispose()
    }
}

function Get-DirectoryPayloadInfo {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$TopLevelName
    )

    $ResolvedRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd("\")
    $Files = @(Get-ChildItem -LiteralPath $ResolvedRoot -Recurse -Force -File | Sort-Object FullName)
    $Rows = [System.Collections.Generic.List[string]]::new()
    foreach ($File in $Files) {
        $RelativePath = $File.FullName.Substring($ResolvedRoot.Length).TrimStart("\").Replace("\", "/")
        $Hash = Get-FileSha256 -Path $File.FullName
        $Rows.Add("$TopLevelName/$RelativePath`t$Hash")
    }
    $Material = [System.Text.Encoding]::UTF8.GetBytes((($Rows -join "`n") + "`n"))
    $TreeHasher = [System.Security.Cryptography.SHA256]::Create()
    try {
        $TreeHash = Convert-BytesToHex -Bytes ($TreeHasher.ComputeHash($Material))
    } finally {
        $TreeHasher.Dispose()
    }
    return [pscustomobject]@{
        entryCount = $Files.Count
        payloadTreeSha256 = $TreeHash
    }
}

function Reset-SafeDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$AllowedParent
    )

    $ResolvedPath = [System.IO.Path]::GetFullPath($Path).TrimEnd("\")
    $ResolvedParent = [System.IO.Path]::GetFullPath($AllowedParent).TrimEnd("\")
    if (-not $ResolvedPath.StartsWith(($ResolvedParent + "\"), [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to reset path outside Fab output root: $ResolvedPath"
    }
    if (Test-Path -LiteralPath $ResolvedPath) {
        Remove-Item -LiteralPath $ResolvedPath -Recurse -Force
    }
}

function Copy-AllowedItem {
    param(
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$DestinationPath
    )

    if (Test-Path -LiteralPath $SourcePath -PathType Container) {
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Recurse -Force
    } elseif (Test-Path -LiteralPath $SourcePath -PathType Leaf) {
        $Parent = Split-Path -Parent $DestinationPath
        if ($Parent) {
            New-Item -ItemType Directory -Force -Path $Parent | Out-Null
        }
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
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

if ($SkipProjectEditorBuild) {
    Write-Verbose "-SkipProjectEditorBuild is retained for compatibility; PackageFabRelease no longer consumes workspace-built editor binaries."
}

foreach ($RequiredPath in @($SourceZipPath, $SourceManifestPath)) {
    if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
        throw "Required exact-source artifact file not found: $RequiredPath"
    }
}

$SourceManifest = Get-Content -LiteralPath $SourceManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($SourceManifest.schema -ne "runtimeinspector.fab-source-manifest.v1") {
    throw "Unsupported source artifact manifest schema: $($SourceManifest.schema)"
}
if ([bool]$SourceManifest.shippingPathDirty) {
    throw "Source artifact manifest reports dirty shipping paths."
}

$CurrentCommitOutput = @(& git -C $PluginRoot rev-parse --verify HEAD 2>&1)
if ($LASTEXITCODE -ne 0 -or $CurrentCommitOutput.Count -eq 0) {
    throw "Unable to resolve current committed HEAD for source provenance validation."
}
$CurrentCommit = ($CurrentCommitOutput[-1]).Trim()
if ([string]$SourceManifest.commit -ne $CurrentCommit) {
    throw "Source artifact commit $($SourceManifest.commit) does not match current committed HEAD $CurrentCommit. Regenerate the source ZIP."
}

$ActualZipHash = Get-FileSha256 -Path $SourceZipPath
if ($ActualZipHash -ne ([string]$SourceManifest.zipSha256).ToLowerInvariant()) {
    throw "Source ZIP SHA-256 does not match its manifest."
}
$ZipPayloadInfo = Get-ZipPayloadInfo -Path $SourceZipPath
if ($ZipPayloadInfo.entryCount -ne [int]$SourceManifest.entryCount -or
    $ZipPayloadInfo.payloadTreeSha256 -ne ([string]$SourceManifest.payloadTreeSha256).ToLowerInvariant()) {
    throw "Source ZIP payload tree does not match its manifest."
}

Reset-SafeDirectory -Path $StageRoot -AllowedParent $FabRoot
Reset-SafeDirectory -Path $OutputRoot -AllowedParent $FabRoot
Reset-SafeDirectory -Path $ValidationRoot -AllowedParent $FabRoot
New-Item -ItemType Directory -Force -Path $StageRoot, $ContractRoot | Out-Null
Expand-Archive -LiteralPath $SourceZipPath -DestinationPath $StageRoot -Force

$TopLevelEntries = @(Get-ChildItem -LiteralPath $StageRoot -Force)
if ($TopLevelEntries.Count -ne 1 -or -not $TopLevelEntries[0].PSIsContainer -or $TopLevelEntries[0].Name -ne $PluginName) {
    throw "Extracted source artifact must contain only the $PluginName directory."
}
$StagedPluginRoot = $TopLevelEntries[0].FullName
$StagedPlugin = Join-Path $StagedPluginRoot "$PluginName.uplugin"

$ExtractedPayloadInfo = Get-DirectoryPayloadInfo -Root $StagedPluginRoot -TopLevelName $PluginName
if ($ExtractedPayloadInfo.entryCount -ne $ZipPayloadInfo.entryCount -or
    $ExtractedPayloadInfo.payloadTreeSha256 -ne $ZipPayloadInfo.payloadTreeSha256) {
    throw "Extracted BuildPlugin input is not byte-identical to the source ZIP payload."
}

Invoke-ArtifactContract -Mode "SourceSubmission" -Root $StagedPluginRoot -Report $SourceContractReportPath

$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$AutomationToolDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll"
$BundledDotNetRoot = Join-Path $EngineRoot "Engine\Binaries\ThirdParty\DotNet"
if (-not (Test-Path -LiteralPath $RunUAT -PathType Leaf)) {
    throw "RunUAT.bat not found at: $RunUAT"
}

$BundledDotNetExe = Join-Path $BundledDotNetRoot "8.0.300\win-x64\dotnet.exe"
if (-not (Test-Path -LiteralPath $BundledDotNetExe -PathType Leaf)) {
    $BundledDotNetExe = Get-ChildItem -LiteralPath $BundledDotNetRoot -Filter dotnet.exe -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if ($BundledDotNetExe) {
    $env:PATH = "$(Split-Path -Parent $BundledDotNetExe);$env:PATH"
}

foreach ($Path in @($LogPath, $StdOutLogPath, $StdErrLogPath)) {
    Remove-Item -LiteralPath $Path -Force -ErrorAction SilentlyContinue
}

$LaunchFilePath = $RunUAT
$LaunchArguments = @(
    "BuildPlugin",
    "-Plugin=$StagedPlugin",
    "-Package=$ValidationRoot",
    "-TargetPlatforms=$TargetPlatforms"
)
if ($NoHostPlatform) {
    $LaunchArguments += "-NoHostPlatform"
}

if ($BundledDotNetExe -and (Test-Path -LiteralPath $AutomationToolDll -PathType Leaf)) {
    $LaunchFilePath = $BundledDotNetExe
    $LaunchArguments = @(
        $AutomationToolDll,
        "BuildPlugin",
        "-Plugin=$StagedPlugin",
        "-Package=$ValidationRoot",
        "-TargetPlatforms=$TargetPlatforms"
    )
    if ($NoHostPlatform) {
        $LaunchArguments += "-NoHostPlatform"
    }
}

$MaxParallelEnvName = "UnrealBuildTool_BuildConfiguration__MaxParallelActions"
$PreviousMaxParallelActions = [Environment]::GetEnvironmentVariable($MaxParallelEnvName, "Process")
try {
    # BuildPlugin does not expose arbitrary UBT arguments in UE 5.7. Use UBT's
    # documented environment-to-XML bridge so only this UAT process and its
    # children inherit the validation concurrency cap.
    [Environment]::SetEnvironmentVariable($MaxParallelEnvName, $MaxParallelActions.ToString(), "Process")
    Write-Host "BuildPlugin UBT MaxParallelActions: $MaxParallelActions"

    $UatProcess = Start-Process `
        -FilePath $LaunchFilePath `
        -ArgumentList $LaunchArguments `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $StdOutLogPath `
        -RedirectStandardError $StdErrLogPath
} finally {
    [Environment]::SetEnvironmentVariable($MaxParallelEnvName, $PreviousMaxParallelActions, "Process")
}

New-Item -ItemType File -Force -Path $LogPath | Out-Null
if (Test-Path -LiteralPath $StdOutLogPath) {
    Get-Content -LiteralPath $StdOutLogPath | Tee-Object -FilePath $LogPath -Append | Out-Host
}
if (Test-Path -LiteralPath $StdErrLogPath) {
    Get-Content -LiteralPath $StdErrLogPath | Tee-Object -FilePath $LogPath -Append | Out-Host
}
if ($UatProcess.ExitCode -ne 0) {
    throw "BuildPlugin failed. See log: $LogPath"
}

$ValidationPluginRootCandidates = @(
    $ValidationRoot,
    (Join-Path $ValidationRoot $PluginName),
    (Join-Path $ValidationRoot "HostProject\Plugins\$PluginName")
)
$ValidationPluginRoot = $null
foreach ($Candidate in $ValidationPluginRootCandidates) {
    if (Test-Path -LiteralPath (Join-Path $Candidate "$PluginName.uplugin") -PathType Leaf) {
        $ValidationPluginRoot = $Candidate
        break
    }
}
if (-not $ValidationPluginRoot) {
    throw "Unable to locate BuildPlugin output under: $ValidationRoot"
}

$FinalPluginRoot = Join-Path $OutputRoot $PluginName
New-Item -ItemType Directory -Force -Path $FinalPluginRoot | Out-Null
foreach ($DirectoryName in @("Binaries", "Config", "Content", "Intermediate", "Resources", "Source")) {
    $SourcePath = Join-Path $ValidationPluginRoot $DirectoryName
    if (Test-Path -LiteralPath $SourcePath -PathType Container) {
        Copy-AllowedItem -SourcePath $SourcePath -DestinationPath (Join-Path $FinalPluginRoot $DirectoryName)
    }
}
foreach ($FileName in @("RuntimeInspector.uplugin", "README.md", "USER_GUIDE_zh-CN.md", "LICENSE", "LICENSE.txt")) {
    $SourcePath = Join-Path $ValidationPluginRoot $FileName
    if (Test-Path -LiteralPath $SourcePath -PathType Leaf) {
        Copy-AllowedItem -SourcePath $SourcePath -DestinationPath (Join-Path $FinalPluginRoot $FileName)
    }
}

$ForbiddenFinalEntries = @(".git", ".agents", "Docs", "HostProject", "Saved")
$LeakedFinalEntries = @(Get-ChildItem -LiteralPath $FinalPluginRoot -Force | Where-Object { $ForbiddenFinalEntries -contains $_.Name })
if ($LeakedFinalEntries.Count -gt 0) {
    throw ("Forbidden entries present in compiled-smoke artifact:`n" + (($LeakedFinalEntries | ForEach-Object { $_.FullName }) -join "`n"))
}

Invoke-ArtifactContract -Mode "CompiledSmoke" -Root $FinalPluginRoot -Report $CompiledContractReportPath
$CompiledPayloadInfo = Get-DirectoryPayloadInfo -Root $FinalPluginRoot -TopLevelName $PluginName
$Provenance = [ordered]@{
    schema = "runtimeinspector.fab-compiled-smoke-provenance.v1"
    generatedUtc = [DateTime]::UtcNow.ToString("o")
    sourceCommit = [string]$SourceManifest.commit
    sourceZip = $SourceZipPath
    sourceZipSha256 = $ActualZipHash
    sourcePayloadTreeSha256 = $ZipPayloadInfo.payloadTreeSha256
    extractedInputPayloadTreeSha256 = $ExtractedPayloadInfo.payloadTreeSha256
    buildPluginTargetPlatforms = $TargetPlatforms
    buildPluginIncludedHostPlatform = (-not [bool]$NoHostPlatform)
    compiledEntryCount = $CompiledPayloadInfo.entryCount
    compiledPayloadTreeSha256 = $CompiledPayloadInfo.payloadTreeSha256
    sourceContractReport = $SourceContractReportPath
    compiledContractReport = $CompiledContractReportPath
    buildLog = $LogPath
}
$Provenance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ProvenancePath -Encoding UTF8

Write-Host ""
Write-Host "[PASS] Exact-source BuildPlugin compiled smoke artifact"
Write-Host "       Source commit: $($SourceManifest.commit)"
Write-Host "       Source ZIP:    $SourceZipPath"
Write-Host "       Source SHA:    $ActualZipHash"
Write-Host "       Build input:   $StagedPluginRoot"
Write-Host "       Build output:  $FinalPluginRoot"
Write-Host "       Source report: $SourceContractReportPath"
Write-Host "       Binary report: $CompiledContractReportPath"
Write-Host "       Provenance:    $ProvenancePath"
Write-Host "       Log:           $LogPath"

if (-not $KeepStage) {
    Reset-SafeDirectory -Path $StageRoot -AllowedParent $FabRoot
    Write-Host "       Cleaned stage: $StageRoot"
}
if (-not $KeepValidationOutput) {
    Reset-SafeDirectory -Path $ValidationRoot -AllowedParent $FabRoot
    Write-Host "       Cleaned UAT output: $ValidationRoot"
}
