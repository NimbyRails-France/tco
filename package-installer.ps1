param(
 [string]$SdkRoot="$PSScriptRoot/../sdk/install/0.7.1/Release",
 [string]$Iscc="$env:LOCALAPPDATA/Programs/InnoSetup/ISCC.exe",
 [string]$FeedUrl='',
 [string]$ReleaseBaseUrl='',
 [string]$QtRoot='C:/Qt/6.11.2/mingw_64',
 [string]$QtTools='C:/Qt/Tools',
 [string]$QtLicenseRoot='C:/Qt/Licenses',
 [string]$BuildDirectory="$PSScriptRoot/build",
 [switch]$SkipBuild
)
$ErrorActionPreference='Stop'

$root=$PSScriptRoot
$tcoBuild=$BuildDirectory
$version='0.5.2'
foreach($url in @($FeedUrl,$ReleaseBaseUrl)) { if($url -and ([uri]$url).Scheme -ne 'https'){throw 'Update URLs must use HTTPS'} }
if(!(Test-Path -LiteralPath $Iscc)){throw "Install Inno Setup 6 and pass -Iscc. Missing: $Iscc"}
if(!$SkipBuild){
 if([IO.Path]::GetFullPath($BuildDirectory) -ne [IO.Path]::GetFullPath("$root/build")){throw 'A custom BuildDirectory requires -SkipBuild'}
 & "$PSScriptRoot/build.ps1" -SdkRoot $SdkRoot -QtRoot $QtRoot -QtTools $QtTools
}
$stage=Join-Path $root ('build/installer-stage-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
Copy-Item -LiteralPath "$tcoBuild/NimbyTco.exe" -Destination $stage
& "$QtRoot/bin/windeployqt.exe" --release --qmldir $PSScriptRoot --dir $stage "$stage/NimbyTco.exe"
if($LASTEXITCODE){throw 'Qt deployment failed'}
Copy-Item -LiteralPath "$SdkRoot/bin/NimbyRailsFranceSDK.dll","$SdkRoot/bin/libwinpthread-1.dll" -Destination $stage
Copy-Item -LiteralPath $SdkRoot -Destination "$stage/SDK" -Recurse
New-Item -ItemType Directory -Path "$stage/licenses" | Out-Null
Copy-Item -LiteralPath $QtLicenseRoot -Destination "$stage/licenses/Qt" -Recurse
Copy-Item -Path "$PSScriptRoot/licenses/Qt/*" -Destination "$stage/licenses/Qt"
Copy-Item -LiteralPath "$QtRoot/sbom" -Destination "$stage/licenses/Qt/sbom" -Recurse
Copy-Item -LiteralPath "$SdkRoot/share/licenses" -Destination "$stage/licenses/SDK" -Recurse
Copy-Item -LiteralPath "$QtTools/mingw1310_64/licenses" -Destination "$stage/licenses/MinGW-Qt" -Recurse
Copy-Item -LiteralPath "$PSScriptRoot/THIRD_PARTY.md","$PSScriptRoot/README.md" -Destination $stage
@{feed=$FeedUrl} | ConvertTo-Json | Set-Content "$stage/update.json" -Encoding UTF8
$output=Join-Path $root 'dist'
& $Iscc "/DStage=$stage" "/DOutput=$output" "/DVersion=$version" "$PSScriptRoot/installer.iss"
if($LASTEXITCODE){throw 'Installer compilation failed'}
$setup=Join-Path $output "NimbyTco-$version-Setup.exe"
$hash=(Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash.ToLowerInvariant()
$manifest=@{schema=1;product='NimbyTco';platform='windows-x64';version=$version;sdkVersion='0.7.1';url='';sha256=$hash;size=(Get-Item -LiteralPath $setup).Length}
if($ReleaseBaseUrl){$manifest.url=$ReleaseBaseUrl.TrimEnd('/')+'/'+[IO.Path]::GetFileName($setup)}
$manifest | ConvertTo-Json | Set-Content "$output/tco-latest.json" -Encoding UTF8
"$hash  $([IO.Path]::GetFileName($setup))" | Set-Content "$output/NimbyTco-SHA256SUMS.txt" -Encoding ascii
Write-Output "Installer: $setup"
Write-Output "Manifest: $output/tco-latest.json"
$portableParent=Join-Path $root ('build/tco-portable-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $portableParent | Out-Null
$portable=Join-Path $portableParent "NimbyTco-$version"
Copy-Item -LiteralPath $stage -Destination $portable -Recurse
# ZipFile reads read-only license files without requesting write access.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zipTemp=Join-Path $output ('portable-'+[guid]::NewGuid().ToString('N')+'.zip')
[IO.Compression.ZipFile]::CreateFromDirectory($portable,$zipTemp,[IO.Compression.CompressionLevel]::Optimal,$true)
Move-Item -LiteralPath $zipTemp -Destination "$output/NimbyTco-$version-windows-x64.zip" -Force
Write-Output "Portable: $output/NimbyTco-$version-windows-x64.zip"
$zip=Get-Item -LiteralPath "$output/NimbyTco-$version-windows-x64.zip"
$zipHash=(Get-FileHash -LiteralPath $zip.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
$project=@{id='tco';name='Nimby TCO';kind='tco';version=$version;url='';sha256=$zipHash;size=$zip.Length;rootFolder="NimbyTco-$version";sdkMin='0.7.1';sdkMaxExclusive='0.8.0';gameSha256=@('fff49ac21720abfc824c2b4f68b862727630eb0db71cfe1f9ea8f685d0db10ae')}
if($ReleaseBaseUrl){$project.url=$ReleaseBaseUrl.TrimEnd('/')+'/'+$zip.Name}
$project | ConvertTo-Json -Depth 4 | Set-Content "$output/project.json" -Encoding UTF8
@("$hash  $([IO.Path]::GetFileName($setup))", "$zipHash  $($zip.Name)") | Set-Content "$output/SHA256SUMS.txt" -Encoding ascii
Write-Output "Hub manifest: $output/project.json"
if(!$FeedUrl){Write-Warning 'No feed configured. Installer works offline; automatic updates need a release feed.'}
