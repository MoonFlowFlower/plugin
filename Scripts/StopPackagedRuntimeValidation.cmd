@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0StopPackagedRuntimeValidation.ps1" %*
