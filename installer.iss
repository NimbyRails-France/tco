#ifndef Stage
  #error Stage is required
#endif
#ifndef Version
  #define Version "0.5.0"
#endif
[Setup]
AppId={{9A75DC32-7B21-4838-8351-A07F9952B708}
AppName=Nimby TCO et SDK
AppVersion={#Version}
DefaultDirName={localappdata}\Programs\NimbyTco
DefaultGroupName=Nimby TCO
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#Output}
OutputBaseFilename=NimbyTco-{#Version}-Setup
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\NimbyTco.exe
CloseApplications=yes
CloseApplicationsFilter=NimbyTco.exe,NimbyRailsFranceSDK.dll
RestartApplications=no
SetupMutex=NimbyTcoInstaller
WizardStyle=modern
[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
[Files]
Source: "{#Stage}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\Nimby TCO"; Filename: "{app}\NimbyTco.exe"
Name: "{group}\Désinstaller Nimby TCO"; Filename: "{uninstallexe}"
[Run]
Filename: "{app}\NimbyTco.exe"; Description: "Lancer Nimby TCO"; Flags: nowait postinstall skipifsilent
Filename: "{app}\NimbyTco.exe"; Flags: nowait runhidden; Check: RelaunchRequested
[Code]
function RelaunchRequested: Boolean;
var I: Integer;
begin
  Result := False;
  if not WizardSilent then exit;
  for I := 1 to ParamCount do
    if CompareText(ParamStr(I), '/RELAUNCH') = 0 then Result := True;
end;
