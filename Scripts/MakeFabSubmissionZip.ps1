[CmdletBinding()]
param(
    [string]$EngineRoot = "D:\Engine\Unreal\5.7.1\UE_5.7",
    [string]$OutputRoot = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = (Resolve-Path (Join-Path $ScriptRoot "..")).Path
$PluginName = "RuntimeInspector"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $PluginRoot "Saved\FabRelease\Submission"
}
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
$StageDir = Join-Path $OutputRoot $PluginName
$ZipPath = Join-Path $OutputRoot "$PluginName-Fab-Submission.zip"
$ManifestPath = Join-Path $OutputRoot "$PluginName-Fab-Submission.manifest.json"
$ContractReportPath = Join-Path $OutputRoot "$PluginName-Fab-Submission.source-contract.json"

$ArchivePathspecs = @(
    "$PluginName.uplugin",
    "README.md",
    "USER_GUIDE_zh-CN.md",
    "Source",
    "Content",
    "Config",
    "Resources",
    ":(exclude)Content/BP_TestVarsActor.uasset",
    ":(exclude)Content/UI/TestMaterial.uasset"
)

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

        $DigestRows = [System.Collections.Generic.List[string]]::new()
        foreach ($Entry in $Files) {
            if ($Entry.FullName.Contains("\") -or $Entry.FullName.StartsWith("/") -or $Entry.FullName -match "(^|/)\.\.(/|$)") {
                throw "Unsafe ZIP entry: $($Entry.FullName)"
            }

            $Stream = $Entry.Open()
            try {
                $Hasher = [System.Security.Cryptography.SHA256]::Create()
                try {
                    $EntryHash = Convert-BytesToHex -Bytes ($Hasher.ComputeHash($Stream))
                } finally {
                    $Hasher.Dispose()
                }
            } finally {
                $Stream.Dispose()
            }
            $DigestRows.Add("$($Entry.FullName)`t$EntryHash")
        }

        $DigestMaterial = [System.Text.Encoding]::UTF8.GetBytes((($DigestRows -join "`n") + "`n"))
        $TreeHasher = [System.Security.Cryptography.SHA256]::Create()
        try {
            $TreeHash = Convert-BytesToHex -Bytes ($TreeHasher.ComputeHash($DigestMaterial))
        } finally {
            $TreeHasher.Dispose()
        }

        return [pscustomobject]@{
            entryCount = $Files.Count
            topLevelDirectory = $TopLevels[0]
            payloadTreeSha256 = $TreeHash
            entries = @($Files | ForEach-Object { $_.FullName })
        }
    } finally {
        $Archive.Dispose()
    }
}

function Reset-SafeDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$AllowedParent
    )

    $ResolvedPath = [System.IO.Path]::GetFullPath($Path).TrimEnd("\")
    $ResolvedParent = [System.IO.Path]::GetFullPath($AllowedParent).TrimEnd("\")
    $Prefix = $ResolvedParent + "\"
    if (-not $ResolvedPath.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to reset directory outside output root: $ResolvedPath"
    }
    if (Test-Path -LiteralPath $ResolvedPath) {
        Remove-Item -LiteralPath $ResolvedPath -Recurse -Force
    }
}

function Invoke-ArtifactContract {
    param(
        [Parameter(Mandatory = $true)][string]$Mode,
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Report
    )

    $ContractScript = Join-Path $ScriptRoot "TestFabArtifactContract.ps1"
    if (-not (Test-Path -LiteralPath $ContractScript -PathType Leaf)) {
        throw "Artifact contract script not found: $ContractScript"
    }
    $PowerShellExe = Join-Path $env:SystemRoot "System32\WindowsPowerShell\v1.0\powershell.exe"
    $Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$ContractScript`" -Mode $Mode -PluginRoot `"$Root`" -ReportPath `"$Report`""
    $Process = Start-Process -FilePath $PowerShellExe -ArgumentList $Arguments -NoNewWindow -Wait -PassThru
    if ($Process.ExitCode -ne 0) {
        throw "Fab artifact contract failed ($Mode). Report: $Report"
    }
}

$GitCommand = Get-Command git -ErrorAction SilentlyContinue
if (-not $GitCommand) {
    throw "git.exe is required to build the exact committed Fab submission ZIP."
}

$RepoRootOutput = @(& git -C $PluginRoot rev-parse --show-toplevel 2>&1)
if ($LASTEXITCODE -ne 0 -or $RepoRootOutput.Count -eq 0) {
    throw "Unable to resolve the Git repository containing: $PluginRoot"
}
$RepoRoot = [System.IO.Path]::GetFullPath(($RepoRootOutput[-1]).Trim())
if (-not $PluginRoot.Equals($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "RuntimeInspector must be the repository root for exact Fab packaging. Repo=$RepoRoot Plugin=$PluginRoot"
}

$CommitOutput = @(& git -C $RepoRoot rev-parse --verify HEAD 2>&1)
if ($LASTEXITCODE -ne 0 -or $CommitOutput.Count -eq 0) {
    throw "Unable to resolve committed HEAD."
}
$Commit = ($CommitOutput[-1]).Trim()

$DirtyEntries = @(& git -C $RepoRoot status --porcelain=v1 --untracked-files=all -- @ArchivePathspecs 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw "Unable to inspect shipping-path worktree state."
}
$DirtyEntries = @($DirtyEntries | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
if ($DirtyEntries.Count -gt 0) {
    throw ("Fab shipping paths are dirty; refusing to create a source artifact that could disagree with committed HEAD {0}:`n{1}" -f $Commit, ($DirtyEntries -join "`n"))
}

$BuildVersionPath = Join-Path $EngineRoot "Engine\Build\Build.version"
if (-not (Test-Path -LiteralPath $BuildVersionPath -PathType Leaf)) {
    throw "UE Build.version not found at: $BuildVersionPath"
}
$BuildVersion = Get-Content -LiteralPath $BuildVersionPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ([int]$BuildVersion.MajorVersion -ne 5 -or [int]$BuildVersion.MinorVersion -ne 7) {
    throw "Fab RC packaging requires UE 5.7; Build.version reports $($BuildVersion.MajorVersion).$($BuildVersion.MinorVersion)."
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$TempZipPath = Join-Path $OutputRoot (".{0}-{1}.zip" -f $PluginName, [Guid]::NewGuid().ToString("N"))
try {
    $GitArguments = @(
        "-C", $RepoRoot,
        "archive",
        "--format=zip",
        "--prefix=$PluginName/",
        "--output=$TempZipPath",
        "HEAD",
        "--"
    ) + $ArchivePathspecs

    & git @GitArguments
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $TempZipPath -PathType Leaf)) {
        throw "git archive failed for committed HEAD $Commit."
    }

    $PayloadInfo = Get-ZipPayloadInfo -Path $TempZipPath
    foreach ($RequiredEntry in @(
        "$PluginName/$PluginName.uplugin",
        "$PluginName/Config/ToolsSelfTestsDefault.json",
        "$PluginName/Config/ToolsWorkflowsDefault.json"
    )) {
        if ($PayloadInfo.entries -notcontains $RequiredEntry) {
            throw "Committed source ZIP is missing required entry: $RequiredEntry"
        }
    }

    if (Test-Path -LiteralPath $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }
    Move-Item -LiteralPath $TempZipPath -Destination $ZipPath -Force
} finally {
    if (Test-Path -LiteralPath $TempZipPath) {
        Remove-Item -LiteralPath $TempZipPath -Force
    }
}

Reset-SafeDirectory -Path $StageDir -AllowedParent $OutputRoot
Expand-Archive -LiteralPath $ZipPath -DestinationPath $OutputRoot -Force
if (-not (Test-Path -LiteralPath (Join-Path $StageDir "$PluginName.uplugin") -PathType Leaf)) {
    throw "Source ZIP extraction did not produce the expected plugin root: $StageDir"
}

Invoke-ArtifactContract -Mode "SourceSubmission" -Root $StageDir -Report $ContractReportPath

$ZipHash = Get-FileSha256 -Path $ZipPath
$Manifest = [ordered]@{
    schema = "runtimeinspector.fab-source-manifest.v1"
    generatedUtc = [DateTime]::UtcNow.ToString("o")
    commit = $Commit
    shippingPathDirty = $false
    dirtyShippingEntries = @()
    topLevelDirectory = $PayloadInfo.topLevelDirectory
    entryCount = $PayloadInfo.entryCount
    payloadTreeSha256 = $PayloadInfo.payloadTreeSha256
    zipFile = [System.IO.Path]::GetFileName($ZipPath)
    zipSha256 = $ZipHash
    ueVersion = [ordered]@{
        major = [int]$BuildVersion.MajorVersion
        minor = [int]$BuildVersion.MinorVersion
        patch = [int]$BuildVersion.PatchVersion
        changelist = [int64]$BuildVersion.Changelist
    }
    sourceContractReport = [System.IO.Path]::GetFileName($ContractReportPath)
}
$Manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ManifestPath -Encoding UTF8

$ZipSizeMB = [math]::Round((Get-Item -LiteralPath $ZipPath).Length / 1MB, 2)
Write-Host ""
Write-Host "[PASS] Exact committed Fab source artifact"
Write-Host "       Commit:  $Commit"
Write-Host "       ZIP:     $ZipPath ($ZipSizeMB MB)"
Write-Host "       SHA-256: $ZipHash"
Write-Host "       Entries: $($PayloadInfo.entryCount)"
Write-Host "       Manifest: $ManifestPath"
Write-Host "       Contract: $ContractReportPath"
