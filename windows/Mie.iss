#define MyAppName "Mie Survival"
#ifndef MyAppVersion
#define MyAppVersion "0.9.6"
#endif
#define MyAppPublisher "Mie contributors"
#define MyAppExeName "Mie.exe"
#define MyAppLauncherName "MieLauncher.exe"

[Setup]
AppId={{7B31E8F2-920D-49B9-81D8-2C2CFE9237AF}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\Mie Survival
DefaultGroupName=Mie Survival
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=Mie-Survival-Windows-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupLogging=yes
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Criar um atalho na área de trabalho"; GroupDescription: "Atalhos:"; Flags: unchecked

[Dirs]
Name: "{app}\playerSettings"
Name: "{app}\resources\worlds"

[Files]
Source: "..\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Mie Survival"; Filename: "{app}\{#MyAppLauncherName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\Mie Survival"; Filename: "{app}\{#MyAppLauncherName}"; WorkingDir: "{app}"; IconFilename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppLauncherName}"; Description: "Executar Mie Survival"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Remove only transient logs. Player-created worlds and settings are intentionally
; not recursively deleted, so reinstall/update cycles do not destroy saves.
Type: files; Name: "{app}\errorLogs.txt"
Type: filesandordirs; Name: "{app}\updates"
