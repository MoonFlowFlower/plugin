[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("SourceSubmission", "CompiledSmoke")]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$PluginRoot,

    [Parameter(Mandatory = $true)]
    [string]$ReportPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Checks = [System.Collections.Generic.List[object]]::new()
$Issues = [System.Collections.Generic.List[string]]::new()

function Add-ContractCheck {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Detail
    )

    $Checks.Add([pscustomobject]@{
        name = $Name
        passed = $Passed
        detail = $Detail
    })
    if (-not $Passed) {
        $Issues.Add("${Name}: ${Detail}")
    }
}

function Resolve-RequiredDirectory {
    param([string]$Root, [string]$RelativePath)
    $Path = Join-Path $Root $RelativePath
    Add-ContractCheck -Name "directory:$RelativePath" -Passed (Test-Path -LiteralPath $Path -PathType Container) -Detail $Path
}

function Read-JsonArray {
    param([string]$Path, [string]$CheckName)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Add-ContractCheck -Name $CheckName -Passed $false -Detail "Missing file: $Path"
        return @()
    }

    try {
        $Value = Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
        $Rows = @($Value)
        Add-ContractCheck -Name $CheckName -Passed ($Rows.Count -gt 0) -Detail "Parsed rows=$($Rows.Count): $Path"
        return $Rows
    } catch {
        Add-ContractCheck -Name $CheckName -Passed $false -Detail "JSON parse failed: $($_.Exception.Message)"
        return @()
    }
}

$ResolvedPluginRoot = $null
try {
    $ResolvedPluginRoot = (Resolve-Path -LiteralPath $PluginRoot -ErrorAction Stop).Path
} catch {
    $ResolvedPluginRoot = [System.IO.Path]::GetFullPath($PluginRoot)
}

$RootExists = Test-Path -LiteralPath $ResolvedPluginRoot -PathType Container
Add-ContractCheck -Name "plugin-root" -Passed $RootExists -Detail $ResolvedPluginRoot

if ($RootExists) {
    foreach ($Directory in @("Source", "Content", "Config", "Resources")) {
        Resolve-RequiredDirectory -Root $ResolvedPluginRoot -RelativePath $Directory
    }

    $DescriptorPath = Join-Path $ResolvedPluginRoot "RuntimeInspector.uplugin"
    Add-ContractCheck -Name "descriptor" -Passed (Test-Path -LiteralPath $DescriptorPath -PathType Leaf) -Detail $DescriptorPath

    $SelfTestsPath = Join-Path $ResolvedPluginRoot "Config\ToolsSelfTestsDefault.json"
    $WorkflowsPath = Join-Path $ResolvedPluginRoot "Config\ToolsWorkflowsDefault.json"
    $SelfTests = Read-JsonArray -Path $SelfTestsPath -CheckName "tools-self-tests-json"
    $Workflows = Read-JsonArray -Path $WorkflowsPath -CheckName "tools-workflows-json"

    $HasDockLayout = @($SelfTests | Where-Object { $_.PSObject.Properties.Name -contains "Id" -and $_.Id -eq "dock_layout" }).Count -gt 0
    $HasMainlineClosure = @($Workflows | Where-Object { $_.PSObject.Properties.Name -contains "WorkflowId" -and $_.WorkflowId -eq "mainline_full_closure" }).Count -gt 0
    Add-ContractCheck -Name "self-test:dock_layout" -Passed $HasDockLayout -Detail "Required Id=dock_layout"
    Add-ContractCheck -Name "workflow:mainline_full_closure" -Passed $HasMainlineClosure -Detail "Required WorkflowId=mainline_full_closure"

    $ForbiddenDirectories = @(".git", ".agents", "Saved", "DerivedDataCache")
    foreach ($Directory in $ForbiddenDirectories) {
        $ForbiddenPath = Join-Path $ResolvedPluginRoot $Directory
        Add-ContractCheck -Name "forbidden-directory:$Directory" -Passed (-not (Test-Path -LiteralPath $ForbiddenPath)) -Detail $ForbiddenPath
    }

    foreach ($FileName in @("code_review.md", "run_verify.sh")) {
        $Matches = @(Get-ChildItem -LiteralPath $ResolvedPluginRoot -Recurse -Force -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -ieq $FileName })
        Add-ContractCheck -Name "forbidden-file:$FileName" -Passed ($Matches.Count -eq 0) -Detail ((@($Matches | ForEach-Object { $_.FullName }) -join "; "))
    }

    $BinariesPath = Join-Path $ResolvedPluginRoot "Binaries"
    $IntermediatePath = Join-Path $ResolvedPluginRoot "Intermediate"
    if ($Mode -eq "SourceSubmission") {
        Add-ContractCheck -Name "source:no-binaries" -Passed (-not (Test-Path -LiteralPath $BinariesPath)) -Detail $BinariesPath
        Add-ContractCheck -Name "source:no-intermediate" -Passed (-not (Test-Path -LiteralPath $IntermediatePath)) -Detail $IntermediatePath
        $CompiledFiles = @(Get-ChildItem -LiteralPath $ResolvedPluginRoot -Recurse -Force -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in @(".dll", ".exe", ".lib", ".pdb") })
        Add-ContractCheck -Name "source:no-compiled-files" -Passed ($CompiledFiles.Count -eq 0) -Detail ((@($CompiledFiles | ForEach-Object { $_.FullName }) -join "; "))
    } else {
        Add-ContractCheck -Name "compiled:binaries-directory" -Passed (Test-Path -LiteralPath $BinariesPath -PathType Container) -Detail $BinariesPath
        $RuntimeDlls = @(Get-ChildItem -LiteralPath $BinariesPath -Recurse -File -Filter "*RuntimeInspector*.dll" -ErrorAction SilentlyContinue)
        Add-ContractCheck -Name "compiled:runtime-binary" -Passed ($RuntimeDlls.Count -gt 0) -Detail ((@($RuntimeDlls | ForEach-Object { $_.FullName }) -join "; "))
    }
}

$EntryCount = if ($RootExists) { @(Get-ChildItem -LiteralPath $ResolvedPluginRoot -Recurse -Force -File -ErrorAction SilentlyContinue).Count } else { 0 }
$Report = [ordered]@{
    schema = "runtimeinspector.fab-artifact-contract.v1"
    generatedUtc = [DateTime]::UtcNow.ToString("o")
    mode = $Mode
    pluginRoot = $ResolvedPluginRoot
    passed = ($Issues.Count -eq 0)
    entryCount = $EntryCount
    checks = @($Checks)
    issues = @($Issues)
}

$ResolvedReportPath = [System.IO.Path]::GetFullPath($ReportPath)
$ReportDirectory = Split-Path -Parent $ResolvedReportPath
if (-not [string]::IsNullOrWhiteSpace($ReportDirectory)) {
    New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
}
$Report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ResolvedReportPath -Encoding UTF8

if ($Report.passed) {
    Write-Host "[PASS] Fab artifact contract ($Mode): $ResolvedPluginRoot"
    Write-Host "       Report: $ResolvedReportPath"
    exit 0
}

Write-Host "[FAIL] Fab artifact contract ($Mode): $ResolvedPluginRoot" -ForegroundColor Red
foreach ($Issue in $Issues) {
    Write-Host "       $Issue" -ForegroundColor Red
}
Write-Host "       Report: $ResolvedReportPath"
exit 1
