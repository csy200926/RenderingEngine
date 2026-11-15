param(
    [string]$SourceProject = 'D:/Projects/PrototypeScene',
    [string]$UnityEditor = 'C:/Program Files/Unity/Hub/Editor/6000.3.22f1/Editor/Unity.exe'
)
$ErrorActionPreference = 'Stop'
$rendererRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$conversionProject = Join-Path $rendererRoot 'Artifacts/HighlandsExportProject'
$packageSource = Join-Path $SourceProject 'Assets/HIGHLANDS - Stylized Environment'
$packageCopy = Join-Path $conversionProject 'Assets/HIGHLANDS - Stylized Environment'
if ([IO.Path]::GetFullPath($conversionProject).StartsWith([IO.Path]::GetFullPath($SourceProject), [StringComparison]::OrdinalIgnoreCase)) {
    throw '转换目录必须位于源 Unity 工程之外。'
}
# 第一步：只复制转换需要的资产，保留 meta GUID 以恢复 prefab 覆盖。
$allowed = @('.fbx', '.png', '.tga', '.jpg', '.jpeg', '.hdr', '.mat', '.prefab', '.terrainlayer', '.asset', '.unity')
$manifest = @()
Get-ChildItem -LiteralPath $packageSource -File -Recurse | Where-Object {
    $allowed -contains $_.Extension.ToLowerInvariant() -and
    $_.FullName -notmatch '\\(Audio|Animation|Timeline|Documentation)\\' -and
    ($_.Extension -ne '.asset' -or $_.FullName -match '\\Demo\\')
} | ForEach-Object {
    $relative = $_.FullName.Substring($packageSource.Length).TrimStart('\', '/')
    $destination = Join-Path $packageCopy $relative
    New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $destination
    if (Test-Path -LiteralPath ($_.FullName + '.meta')) {
        Copy-Item -LiteralPath ($_.FullName + '.meta') -Destination ($destination + '.meta')
    }
    $manifest += [PSCustomObject]@{ path = $relative.Replace('\', '/'); bytes = $_.Length; sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant() }
}
New-Item -ItemType Directory -Force (Join-Path $conversionProject 'Packages'), (Join-Path $conversionProject 'ProjectSettings'), (Join-Path $conversionProject 'Assets/Editor'), (Join-Path $rendererRoot 'Assets/Highlands') | Out-Null
[IO.File]::WriteAllText((Join-Path $conversionProject 'Packages/manifest.json'), '{"dependencies":{"com.unity.modules.terrain":"1.0.0","com.unity.modules.terrainphysics":"1.0.0","com.unity.modules.imageconversion":"1.0.0","com.unity.modules.physics":"1.0.0","com.unity.modules.particlesystem":"1.0.0","com.unity.modules.audio":"1.0.0","com.unity.modules.animation":"1.0.0"}}', [Text.UTF8Encoding]::new($false))
Set-Content (Join-Path $conversionProject 'ProjectSettings/ProjectVersion.txt') 'm_EditorVersion: 6000.3.22f1'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'HighlandsExporter.cs') -Destination (Join-Path $conversionProject 'Assets/Editor/HighlandsExporter.cs')
ConvertTo-Json -InputObject $manifest -Depth 3 | Set-Content -Encoding utf8 (Join-Path $rendererRoot 'Assets/Highlands/source-manifest.json')
# 第二步：Unity 仅解析一次，运行时只使用 Lua / OBJ / PNG / terrain。
$env:HIGHLANDS_SOURCE_PROJECT = $SourceProject
$log = Join-Path $PSScriptRoot 'export.log'
$process = Start-Process -FilePath $UnityEditor -ArgumentList @('-batchmode', '-nographics', '-quit', '-projectPath', $conversionProject, '-executeMethod', 'HighlandsExporter.Run', '-logFile', $log) -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "Unity 导出失败，退出码 $($process.ExitCode)，见 $log" }
if (-not (Select-String -LiteralPath $log -SimpleMatch 'HIGHLANDS_EXPORT_SUCCESS' -Quiet)) { throw "Unity 未产生成功标记，见 $log" }
Write-Output 'Highlands 离线资源导出完成。'
