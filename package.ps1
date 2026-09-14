param(
 [Parameter(Mandatory=$true)][string]$SdkRoot,
 [string]$QtRoot='C:/Qt/6.11.2/mingw_64',
 [string]$QtTools='C:/Qt/Tools'
)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/build.ps1" -SdkRoot $SdkRoot -QtRoot $QtRoot -QtTools $QtTools
$stage=Join-Path $PSScriptRoot 'dist/NimbyTco-0.2.0'
if(Test-Path -LiteralPath $stage){throw "Package folder already exists: $stage. Use a fresh checkout or move the old package first."}
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item -LiteralPath "$PSScriptRoot/build/NimbyTco.exe" -Destination $stage
Copy-Item -LiteralPath "$SdkRoot/bin/NimbyRailsSDK.dll" -Destination $stage
& "$QtRoot/bin/windeployqt.exe" --release --qmldir $PSScriptRoot --dir $stage "$stage/NimbyTco.exe"
if($LASTEXITCODE){throw 'Qt package deployment failed'}
Copy-Item -LiteralPath "$SdkRoot/bin/libwinpthread-1.dll" -Destination $stage -Force
Copy-Item -LiteralPath "$PSScriptRoot/licenses" -Destination $stage -Recurse
Copy-Item -LiteralPath "$SdkRoot/share/licenses" -Destination "$stage/licenses/SDK" -Recurse
Copy-Item -LiteralPath "$QtTools/mingw1310_64/licenses" -Destination "$stage/licenses/MinGW-Qt" -Recurse
Copy-Item "$PSScriptRoot/README.md","$PSScriptRoot/THIRD_PARTY.md" -Destination $stage
$zip=Join-Path $PSScriptRoot 'dist/NimbyTco-0.2.0-windows-x64.zip'
Compress-Archive -LiteralPath $stage -DestinationPath $zip -Force
$hash=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $([IO.Path]::GetFileName($zip))" | Set-Content "$PSScriptRoot/dist/SHA256SUMS.txt" -Encoding ascii
Write-Output "Release asset: $zip"