param([string]$NativePackageVersion='')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/package.ps1" -NativePackageVersion $NativePackageVersion
