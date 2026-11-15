@echo off
if not exist "%~dp0Build\bin\Release\Renderer.exe" call "%~dp0Build.cmd"
if errorlevel 1 exit /b %ERRORLEVEL%
"%~dp0Build\bin\Release\Renderer.exe" --renderer dx11 %*
