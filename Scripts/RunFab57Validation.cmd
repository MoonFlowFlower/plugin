@echo off
rem One-click UE 5.7 Fab validation:
rem   1) BuildPlugin package build (PackageFabRelease.ps1)
rem   2) Blank-project load validation (ValidateFabBlankProject.ps1 -KeepValidationProject)
setlocal
set ENGINE=D:\Engine\Unreal\5.7.1\UE_5.7

if not exist "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" (
    echo [ERROR] UE 5.7 not found at %ENGINE%
    echo Edit this file and set ENGINE to your UE 5.7 install path.
    pause
    exit /b 1
)

echo === Step 1/2: BuildPlugin package build (UE 5.7) ===
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PackageFabRelease.ps1" -EngineRoot "%ENGINE%"
if errorlevel 1 (
    echo [FAIL] Package build failed. See Saved\build_runtimeinspector_fab_release*.log
    pause
    exit /b 1
)

echo === Step 2/2: Blank project load validation (UE 5.7) ===
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ValidateFabBlankProject.ps1" -EngineRoot "%ENGINE%" -KeepValidationProject
if errorlevel 1 (
    echo [FAIL] Blank project validation failed. See Saved\fab_blank_project_validation.log
    pause
    exit /b 1
)

echo.
echo [OK] UE 5.7 package build and blank-project validation passed.
pause
