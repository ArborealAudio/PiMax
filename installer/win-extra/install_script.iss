[Setup]
AppName={#name}
AppVersion={#version}
VersionInfoVersion={#version}
VersionInfoProductTextVersion={#version}
DisableDirPage=yes
LicenseFile=EULA.txt
UsePreviousAppDir=yes
DefaultDirName={autocf64}
DefaultGroupName={#name}
DisableProgramGroupPage=yes
DisableWelcomePage=no
OutputBaseFilename={#name}-windows
AppPublisher=Arboreal Audio, LLC
AppPublisherURL=https://arborealaudio.com
AppCopyright=(c) 2023 Arboreal Audio, LLC
SetupIconFile=icon.ico
ArchitecturesInstallIn64BitMode=x64
Uninstallable=yes
UninstallFilesDir="{commonappdata}\Arboreal Audio\{#name}"
SolidCompression=yes

[Types]
//Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "VST3"; Description: "{#name} VST3"; Types: custom
Name: "VST"; Description: "{#name} VST2.4"; Types: custom
Name: "CLAP"; Description: "{#name} CLAP"; Types: custom
Name: "Preset"; Description: "Factory Presets"; Types: custom
Name: "Manual"; Description: "User Manual"; Types: custom

[Files]
Source: "{#name}.dll"; DestDir: "{code:GetDir|0}"; Components: VST; Flags: ignoreversion recursesubdirs
Source: "{#name}.vst3"; DestDir: "{code:GetDir|1}"; Components: VST3; Flags: ignoreversion recursesubdirs
Source: "{#name}.clap"; DestDir: "{code:GetDir|2}"; Components: CLAP; Flags: ignoreversion recursesubdirs
Source: "Factory\*"; DestDir: "{userappdata}\Arboreal Audio\{#name}\Presets\Factory"; Components: Preset; Flags: ignoreversion recursesubdirs
Source: "{#name}_manual.pdf"; DestDir: "{userappdata}\Arboreal Audio\{#name}"; Components: Manual; Flags: ignoreversion

[Code]
var
  DirPage: TInputDirWizardPage;
function GetDir(Param: String): String;
begin
  Result := DirPage.Values[StrToInt(Param)];
end;

procedure InitializeWizard;
begin
  DirPage := CreateInputDirPage(
    wpSelectComponents, 'Choose Installation Locations', '', '', False, '');
  DirPage.Add('VST Install Location');
  DirPage.Add('VST3 Install Location');
  DirPage.Add('CLAP Install Location');

  DirPage.Values[0] := GetPreviousData('Directory1', 'C:\Program Files\Steinberg\VSTPlugins');
  DirPage.Values[1] := GetPreviousData('Directory2', 'C:\Program Files\Common Files\VST3');
  DirPage.Values[2] := GetPreviousData('Directory3', 'C:\Program Files\Common Files\CLAP');
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'Directory1', DirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'Directory2', DirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'Directory3', DirPage.Values[2]);
end;