; PatVerb installer (Inno Setup 6). Built by tools/package.ps1 (passes /DAppVersion and /DSourceDir).
#ifndef AppVersion
  #define AppVersion "3.1.4"
#endif
#ifndef SourceDir
  #define SourceDir "..\build\dist\PatVerb_artefacts\Release"
#endif

[Setup]
AppId={{8A2F6D14-5C7B-4E91-B3A8-1F0E9D2C6B57}
AppName=PatVerb
AppVersion={#AppVersion}
AppPublisher=PatVerb
DefaultDirName={autopf}\PatVerb
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=..\dist
OutputBaseFilename=PatVerb-{#AppVersion}-Setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
LicenseFile=..\LICENSE
UninstallDisplayName=PatVerb (VST3 + Standalone reverb)

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; The VST3 goes where every DAW (FL Studio, Ableton, Reaper...) looks for it.
Source: "{#SourceDir}\VST3\PatVerb.vst3\*"; DestDir: "{commoncf64}\VST3\PatVerb.vst3"; Flags: ignoreversion recursesubdirs
Source: "{#SourceDir}\Standalone\PatVerb.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\PatVerb"; Filename: "{app}\PatVerb.exe"

[Messages]
spanish.FinishedLabel=PatVerb quedó instalado. En FL Studio: Options > Manage plugins > Find installed plugins, y marcá la estrella de PatVerb.
english.FinishedLabel=PatVerb is installed. Rescan plugins in your DAW to find it.
