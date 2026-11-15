param([ValidateSet('Release','Debug')][string]$Configuration='Release', [string]$BuildDirectory='Build', [switch]$Package)
$ErrorActionPreference='Stop'
$taskVswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $taskVswhere)) { throw 'Install Visual Studio 2022 with Desktop development with C++ and a Windows SDK.' }
$taskVs=& $taskVswhere -latest -version '[17.0,18.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$taskVs) { throw 'Visual Studio 2022 C++ tools were not found. Add the Desktop development with C++ workload.' }
$taskCmake=Join-Path $taskVs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
if (!(Test-Path -LiteralPath $taskCmake)) { $taskCmake=(Get-Command cmake -ErrorAction Stop).Source }
$taskBuild=if ([IO.Path]::IsPathRooted($BuildDirectory)) {$BuildDirectory} else {Join-Path $PSScriptRoot $BuildDirectory}
& $taskCmake -S $PSScriptRoot -B $taskBuild -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_GENERATOR_INSTANCE=$taskVs"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $taskCmake --build $taskBuild --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Ready: $taskBuild/bin/$Configuration/Renderer.exe --renderer opengl|dx11|dx12|vulkan; Editor.exe --renderer vulkan"
if ($Package) {
    & $taskCmake --install $taskBuild --config $Configuration --prefix (Join-Path $PSScriptRoot 'Output')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
