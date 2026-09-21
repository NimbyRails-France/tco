param(
 [string]$SdkRoot="$PSScriptRoot/../sdk/install/0.7.1/Release",
 [string]$Iscc="$env:LOCALAPPDATA/Programs/InnoSetup/ISCC.exe",
 [string]$QtRoot='C:/Qt/6.11.2/mingw_64',
 [string]$QtTools='C:/Qt/Tools',
 [string]$FeedUrl='https://github.com/NimbyRails-France/tco/releases/latest/download/tco-latest.json',
 [string]$ReleaseBaseUrl='https://github.com/NimbyRails-France/tco/releases/download/v0.5.2'
)
$ErrorActionPreference='Stop'
# One packaging implementation for the portable archive and installer.
& "$PSScriptRoot/package-installer.ps1" -SdkRoot $SdkRoot -Iscc $Iscc -QtRoot $QtRoot -QtTools $QtTools -FeedUrl $FeedUrl -ReleaseBaseUrl $ReleaseBaseUrl