param([ValidateSet('Release','Debug')][string]$Configuration='Release', [switch]$Package, [string]$CMakePath='')
$ErrorActionPreference='Stop'
# vswhere 查询实际安装位置；不把当前机器的 E:/Program Files/VS 写进工程。
$taskVswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $taskVswhere)) { throw 'Install Visual Studio 2022 C++ tools, Windows SDK and CMake component.' }
$taskVs=& $taskVswhere -latest -version '[17.0,18.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$taskVs) { throw 'Visual Studio 2022 Desktop development with C++ was not found.' }
$taskCmake=$CMakePath
if (!$taskCmake) {
    $taskCandidates=@((Join-Path $taskVs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'), (Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'))
    $taskCmake=$taskCandidates | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
}
if (!$taskCmake -or !(Test-Path -LiteralPath $taskCmake)) { throw 'Install CMake tools in Visual Studio, standalone CMake, or pass -CMakePath C:/your/path/cmake.exe.' }
$taskBuild=Join-Path $PSScriptRoot 'Build'
# 整个文件夹移走后，旧 CMakeCache 仍记着原路径。另建目录，不删除用户已有构建。
$taskSuffix=1
while (Test-Path -LiteralPath (Join-Path $taskBuild 'CMakeCache.txt')) {
    $taskCache=Get-Content -LiteralPath (Join-Path $taskBuild 'CMakeCache.txt')
    $taskHomeLine=$taskCache | Where-Object {$_ -match '^CMAKE_HOME_DIRECTORY:[^=]+='} | Select-Object -First 1
    $taskVsLine=$taskCache | Where-Object {$_ -match '^CMAKE_GENERATOR_INSTANCE:[^=]+='} | Select-Object -First 1
    $taskOldHome=if ($taskHomeLine) {($taskHomeLine -split '=',2)[1].Replace('\','/').TrimEnd('/')} else {''}
    $taskOldVs=if ($taskVsLine) {($taskVsLine -split '=',2)[1].Replace('\','/').TrimEnd('/')} else {''}
    if ($taskOldHome -eq $PSScriptRoot.Replace('\','/').TrimEnd('/') -and $taskOldVs -eq $taskVs.Replace('\','/').TrimEnd('/')) { break }
    $taskSuffix++
    $taskBuild=Join-Path $PSScriptRoot "Build-$taskSuffix"
}
& $taskCmake -S $PSScriptRoot -B $taskBuild -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_GENERATOR_INSTANCE=$taskVs"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $taskCmake --build $taskBuild --config $Configuration --parallel 8
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($Package) {
    & $taskCmake --install $taskBuild --config $Configuration --prefix (Join-Path $PSScriptRoot 'Output')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Write-Host "Ready: $taskBuild/bin/$Configuration/RendererLite.exe --renderer vulkan|dx12"
