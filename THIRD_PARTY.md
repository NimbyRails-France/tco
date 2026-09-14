# Third-party components

Nimby TCO uses Qt 6.11.2 (Copyright The Qt Company Ltd. and other contributors),
dynamically linked under LGPL-3.0. License texts and component/copyright inventories
are in licenses/Qt. QtEntryPoint uses its upstream BSD license. Qt libraries may
be replaced by interface-compatible builds; modification and reverse engineering
for debugging modifications to these libraries are permitted.

Corresponding Qt source (including bundled third-party components):
https://github.com/qt/qtbase/tree/v6.11.2
https://github.com/qt/qtdeclarative/tree/v6.11.2
https://github.com/qt/qtsvg/tree/v6.11.2
https://github.com/qt/qttranslations/tree/v6.11.2
https://download.qt.io/official_releases/qt/6.11/6.11.2/submodules/
Build/install details: https://doc.qt.io/qt-6/windows-building.html

MinGW runtime notices are in licenses/MinGW-Qt (GCC 13.1) and
licenses/SDK (SDK GCC 15.2 runtimes and MinHook). GCC runtime libraries use the
upstream runtime exception; see the included license texts.
MinHook is incorporated in NimbyRailsSDK and retains its upstream license.
SDK source: https://github.com/NimbyRails-France/sdk/tree/v0.4.0
Qt itself and NIMBY Rails are separate projects; this is an experimental community tool.