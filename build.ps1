param([string]$QtRoot='C:/Qt/6.11.2/mingw_64', [string]$QtTools='C:/Qt/Tools', [string]$SdkRoot="$PSScriptRoot/../sdk/dist/NimbyRailsFranceSDK-0.7.0")
$ErrorActionPreference='Stop'
$cmake=Join-Path $QtTools 'CMake_64/bin/cmake.exe'
$compiler=Join-Path $QtTools 'mingw1310_64/bin/g++.exe'
$ninja=Join-Path $QtTools 'Ninja/ninja.exe'
$output=Join-Path $PSScriptRoot 'build'
$env:PATH="$(Join-Path $QtTools 'mingw1310_64/bin');$QtRoot/bin;$env:PATH"
& $cmake -S $PSScriptRoot -B $output -G Ninja "-DCMAKE_CXX_COMPILER=$compiler" "-DCMAKE_MAKE_PROGRAM=$ninja" '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_PREFIX_PATH=$QtRoot;$SdkRoot" "-DNimbyRailsFranceSDK_DIR=$SdkRoot/lib/cmake/NimbyRailsFranceSDK"
if($LASTEXITCODE){throw 'TCO configuration failed'}
& $cmake --build $output
if($LASTEXITCODE){throw 'TCO build failed'}
$env:PATH="$(Join-Path $QtTools 'mingw1310_64/bin');$QtRoot/bin;$env:PATH"
& "$QtRoot/bin/windeployqt.exe" --release --qmldir $PSScriptRoot "$output/NimbyTco.exe"
if($LASTEXITCODE){throw 'Qt deployment failed'}
# SDK GCC 15 imports clock_gettime64, absent from Qt kit GCC 13 winpthreads.
# Use the SDK's newer runtime; do not replace Qt's libstdc++ / libgcc DLLs.
Copy-Item -LiteralPath "$SdkRoot/bin/libwinpthread-1.dll" -Destination "$output/libwinpthread-1.dll" -Force
Write-Output "Ready: $output/NimbyTco.exe"
