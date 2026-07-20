# Builds the source-only Fab submission zip for RuntimeInspector.
# Fab code plugins must be uploaded as source (uplugin + Source + Content + Config + Resources).
# Epic's toolchain compiles the binaries; Binaries/Intermediate/Saved must NOT be included.

$ErrorActionPreference = "Stop"

$PluginRoot = Split-Path -Parent $PSScriptRoot
$PluginName = "RuntimeInspector"
$StageRoot  = Join-Path $PluginRoot "Saved\FabRelease\Submission"
$StageDir   = Join-Path $StageRoot $PluginName
$ZipPath    = Join-Path $StageRoot "$PluginName-Fab-Submission.zip"

# Repo-only content that must not ship.
# NOTE: Content\Test is intentionally KEPT — the built-in self-tests load
# /RuntimeInspector/Test/BP_TestVarsActor, MI_Test and M_Test at runtime.
# The two entries below are an unreferenced duplicate and an unused material.
$ContentExcludes = @(
    "Content\BP_TestVarsActor.uasset",
    "Content\UI\TestMaterial.uasset"
)

Write-Host "Staging from: $PluginRoot"
if (Test-Path $StageDir) { Remove-Item $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

# 1. Top-level files
Copy-Item (Join-Path $PluginRoot "$PluginName.uplugin") $StageDir
foreach ($Doc in @("README.md", "USER_GUIDE_zh-CN.md")) {
    $Src = Join-Path $PluginRoot $Doc
    if (Test-Path $Src) { Copy-Item $Src $StageDir }
}

# 2. Whitelisted directories
foreach ($Dir in @("Source", "Content", "Config", "Resources")) {
    $Src = Join-Path $PluginRoot $Dir
    if (Test-Path $Src) {
        Copy-Item $Src (Join-Path $StageDir $Dir) -Recurse
    } else {
        Write-Warning "Missing expected directory: $Dir"
    }
}

# 3. Remove excluded content
foreach ($Rel in $ContentExcludes) {
    $Target = Join-Path $StageDir $Rel
    if (Test-Path $Target) {
        Remove-Item $Target -Recurse -Force
        Write-Host "Excluded: $Rel"
    }
}

# 4. Strip any build residue that slipped in (defensive)
Get-ChildItem $StageDir -Recurse -Directory -Force |
    Where-Object { $_.Name -in @("Binaries", "Intermediate", "Saved", ".git", ".svn", ".agents", ".codex") } |
    ForEach-Object { Remove-Item $_.FullName -Recurse -Force }

# 5. Sanity checks
$Required = @("$PluginName.uplugin", "Source", "Content", "Config", "Resources")
foreach ($Item in $Required) {
    if (-not (Test-Path (Join-Path $StageDir $Item))) {
        throw "Submission package is missing required item: $Item"
    }
}
$Forbidden = Get-ChildItem $StageDir -Recurse -Force |
    Where-Object { $_.Name -match "^(Binaries|Intermediate|Saved)$" -or $_.Extension -in @(".pdb", ".dll", ".lib", ".obj", ".log") }
if ($Forbidden) {
    $Forbidden | ForEach-Object { Write-Warning "Forbidden file in package: $($_.FullName)" }
    throw "Submission package contains forbidden build artifacts."
}

# 6. Zip
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Compress-Archive -Path $StageDir -DestinationPath $ZipPath -CompressionLevel Optimal

$ZipSizeMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Host ""
Write-Host "OK: $ZipPath ($ZipSizeMB MB)"
Write-Host "Upload this zip to a public download link (Google Drive/Dropbox, no login required) for the Fab listing."
