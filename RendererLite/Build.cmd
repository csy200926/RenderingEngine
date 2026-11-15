@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build.ps1" -Package %*
if errorlevel 1 pause
