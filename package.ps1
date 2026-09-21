param([string]$NativePackageVersion='')
$ErrorActionPreference='Stop'
$arguments=@('-p',$PSScriptRoot,'prepareRelease','--console=plain')
if($NativePackageVersion){$arguments+="-PnativePackageVersion=$NativePackageVersion"}
& "$PSScriptRoot/gradlew.bat" @arguments
if($LASTEXITCODE){throw 'TCO Kotlin packaging failed'}
