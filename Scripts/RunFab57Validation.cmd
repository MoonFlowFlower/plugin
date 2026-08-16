@echo off
rem One-click UE 5.7 Fab artifact validation:
rem   1) exact committed source ZIP + SourceSubmission contract
rem   2) BuildPlugin from that ZIP + CompiledSmoke contract
rem   3) blank-host install/load smoke
setlocal
set "ENGINE=D:\Engine\Unreal\5.7.1\UE_5.7"
for %%I in ("%~dp0..") do set "PLUGIN_ROOT=%%~fI"
for %%I in ("%PLUGIN_ROOT%\..\..") do set "PROJECT_ROOT=%%~fI"
set "SOURCE_ZIP=%PLUGIN_ROOT%\Saved\FabRelease\Submission\RuntimeInspector-Fab-Submission.zip"
set "SOURCE_MANIFEST=%PLUGIN_ROOT%\Saved\FabRelease\Submission\RuntimeInspector-Fab-Submission.manifest.json"
set "PACKAGE_ROOT=%PROJECT_ROOT%\Saved\FabRelease\Package\RuntimeInspector_UE57\RuntimeInspector"

if not exist "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" (
    echo [ERROR] UE 5.7 not found at %ENGINE%
    exit /b 1
)

rem A running editor can lock host binaries and invalidate the cold BuildPlugin smoke.
tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo [ERROR] UnrealEditor.exe is running. Close editor and PIE windows, then retry.
    exit /b 1
)

echo === Step 1/3: exact committed source ZIP + contract ===
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0MakeFabSubmissionZip.ps1" -EngineRoot "%ENGINE%"
if errorlevel 1 (
    echo [FAIL] Source ZIP or SourceSubmission contract failed.
    exit /b 1
)

echo === Step 2/3: BuildPlugin from exact source ZIP + contract ===
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PackageFabRelease.ps1" -EngineRoot "%ENGINE%" -SourceZipPath "%SOURCE_ZIP%" -SourceManifestPath "%SOURCE_MANIFEST%"
if errorlevel 1 (
    echo [FAIL] BuildPlugin or CompiledSmoke contract failed. See Saved\FabRelease\build_runtimeinspector_fab_release_UE57*.log
    exit /b 1
)

echo === Step 3/3: RIFabBlank install/load smoke ===
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ValidateFabBlankProject.ps1" -EngineRoot "%ENGINE%" -PackageRoot "%PACKAGE_ROOT%" -KeepValidationProject
if errorlevel 1 (
    echo [FAIL] Blank-host install/load smoke failed. See Saved\FabRelease\fab_blank_host_install_load_smoke_UE57.log
    exit /b 1
)

echo.
echo [PASS] Exact source contract, BuildPlugin contract, and blank-host install/load smoke passed.
exit /b 0
