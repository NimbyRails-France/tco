#!/bin/sh
set -eu
python3 .woodpecker/check-release.py
# Match the SDK 0.7.1 headers used by this TCO's dynamic bridge and packaging.
git init .ci/sdk
git -C .ci/sdk remote add origin https://github.com/NimbyRails-France/sdk.git
git -C .ci/sdk fetch --depth=1 origin 9a9cb086703e3da37bfcb63b2689f2a83ecb20cd
git -C .ci/sdk checkout --detach FETCH_HEAD
cmake -S .ci/sdk -B .ci/sdk/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/nimby-ci/nimby-mingw.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DNIMBY_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX="$PWD/.ci/sdk/install"
cmake --build .ci/sdk/build --parallel 2
cmake --install .ci/sdk/build
cmake -S . -B build/ci -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/nimby-ci/nimby-mingw.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DNRF_DEPLOY_QT=OFF -DNimbyRailsFranceSDK_DIR="$PWD/.ci/sdk/install/lib/cmake/NimbyRailsFranceSDK"
cmake --build build/ci --parallel 2
xvfb-run -a ctest --test-dir build/ci --output-on-failure --timeout 90
