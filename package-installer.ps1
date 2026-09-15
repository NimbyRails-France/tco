param(
 [string]$SdkRoot="$PSScriptRoot/../../dist/NimbyRailsSDK-0.6.0",
 [string]$Iscc="$PSScriptRoot/../../build/installer-tools/Inno/ISCC.exe",
 [string]$FeedUrl='',
 [string]$ReleaseBaseUrl='',
 [string]$QtRoot='C:/Qt/6.11.2/mingw_64',
 [string]$QtTools='C:/Qt/Tools',
 [string]$QtLicenseRoot='C:/Qt/Licenses'
)
$ErrorActionPreference='Stop'
$inSdkTree=Test-Path "$PSScriptRoot/../../include/nimby/sdk.h"
$root=if($inSdkTree){[IO.Path]::GetFullPath("$PSScriptRoot/../..")}else{$PSScriptRoot}
$tcoBuild=if($inSdkTree){Join-Path $root 'build/tco'}else{Join-Path $root 'build'}
$version='0.4.0'
foreach($url in @($FeedUrl,$ReleaseBaseUrl)) { if($url -and ([uri]$url).Scheme -ne 'https'){throw 'Update URLs must use HTTPS'} }
if(!(Test-Path -LiteralPath $Iscc)){throw "Install Inno Setup 6 and pass -Iscc. Missing: $Iscc"}
& "$PSScriptRoot/build.ps1" -SdkRoot $SdkRoot -QtRoot $QtRoot -QtTools $QtTools
$stage=Join-Path $root ('build/installer-stage-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
Copy-Item -LiteralPath "$tcoBuild/NimbyTco.exe" -Destination $stage
& "$QtRoot/bin/windeployqt.exe" --release --qmldir $PSScriptRoot --dir $stage "$stage/NimbyTco.exe"
if($LASTEXITCODE){throw 'Qt deployment failed'}
Copy-Item -LiteralPath "$SdkRoot/bin/NimbyRailsSDK.dll","$SdkRoot/bin/libwinpthread-1.dll" -Destination $stage
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
$manifest=@{schema=1;product='NimbyTco';platform='windows-x64';version=$version;sdkVersion='0.6.0';url='';sha256=$hash;size=(Get-Item -LiteralPath $setup).Length}
if($ReleaseBaseUrl){$manifest.url=$ReleaseBaseUrl.TrimEnd('/')+'/'+[IO.Path]::GetFileName($setup)}
$manifest | ConvertTo-Json | Set-Content "$output/tco-latest.json" -Encoding UTF8
"$hash  $([IO.Path]::GetFileName($setup))" | Set-Content "$output/NimbyTco-SHA256SUMS.txt" -Encoding ascii
Write-Output "Installer: $setup"
Write-Output "Manifest: $output/tco-latest.json"
$portableParent=Join-Path $root ('build/tco-portable-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $portableParent | Out-Null
$portable=Join-Path $portableParent "NimbyTco-$version"
Copy-Item -LiteralPath $stage -Destination $portable -Recurse
Compress-Archive -LiteralPath $portable -DestinationPath "$output/NimbyTco-$version-windows-x64.zip" -Force
Write-Output "Portable: $output/NimbyTco-$version-windows-x64.zip"
if(!$FeedUrl){Write-Warning 'No feed configured. Installer works offline; automatic updates need a release feed.'}
