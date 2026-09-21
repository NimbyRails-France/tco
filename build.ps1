param()
$ErrorActionPreference='Stop'
& "$PSScriptRoot/gradlew.bat" -p $PSScriptRoot desktopTest createDistributable '--console=plain'
if($LASTEXITCODE){throw 'TCO Kotlin build failed'}
