@echo off
setlocal
set "APP=%~dp0RendererLite.exe"
if not exist "%APP%" set "APP=%~dp0Output\RendererLite.exe"
if not exist "%APP%" (
    echo Run Build.cmd first.
    pause
    exit /b 1
)
"%APP%" --renderer vulkan %*
if errorlevel 1 pause
