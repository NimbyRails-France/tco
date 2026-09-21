#!/bin/sh
set -eu
python3 .woodpecker/check-release.py
if [ ! -d .ci/sdk/.git ]; then
  git clone https://github.com/NimbyRails-France/sdk.git .ci/sdk
fi
git -C .ci/sdk checkout --detach "$(cat .woodpecker/sdk-revision.txt)"
xvfb-run -a bash gradlew desktopTest -PnrfSdkClientDir="$PWD/.ci/sdk/kotlin-client" --console=plain
