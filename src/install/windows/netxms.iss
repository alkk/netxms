; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=NetXMS
AppVerName=NetXMS 0.2.19
AppVersion=0.2.19
AppPublisher=NetXMS Team
AppPublisherURL=http://www.netxms.org
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\NetXMS
DefaultGroupName=NetXMS
AllowNoIcons=yes
LicenseFile=..\..\..\copying
OutputBaseFilename=netxms-0.2.19
Compression=lzma
SolidCompression=yes
LanguageDetectionMethod=none

[Components]
Name: "base"; Description: "Base Files"; Types: full compact custom; Flags: fixed
Name: "console"; Description: "Administrator's Console"; Types: full
Name: "server"; Description: "NetXMS Server"; Types: full compact
Name: "server\mssql"; Description: "Microsoft SQL DB-Library"; Types: full
Name: "server\mysql"; Description: "MySQL Client Library"; Types: full
Name: "server\pgsql"; Description: "PostgreSQL Client Library"; Types: full
Name: "server\oracle"; Description: "Oracle Instant Client"; Types: full
Name: "websrv"; Description: "Web Server"; Types: full
Name: "pdb"; Description: "Install PDB files for selected components"; Types: custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Common files
Source: "..\..\..\ChangeLog"; DestDir: "{app}\doc"; Flags: ignoreversion; Components: base
Source: "..\..\libnetxms\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: base
Source: "..\..\libnetxms\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\libnetxms\Release_UNICODE\libnetxmsw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\libnetxms\Release_UNICODE\libnetxmsw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
; Executables and DLLs shared between different components (server, console, etc.)
Source: "..\..\libnxcl\Release\libnxcl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console websrv
Source: "..\..\libnxcl\Release\libnxcl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (console or websrv) and pdb
Source: "..\..\libnxcl\Release_UNICODE\libnxclw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\libnxcl\Release_UNICODE\libnxclw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\libnxmap\Release\libnxmap.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console server websrv
Source: "..\..\libnxmap\Release\libnxmap.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (console or server or websrv) and pdb
Source: "..\..\libnxmap\Release_UNICODE\libnxmapw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\libnxmap\Release_UNICODE\libnxmapw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\libnxsnmp\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\libnxsnmp\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\libnxsnmp\Release_UNICODE\libnxsnmpw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\libnxsnmp\Release_UNICODE\libnxsnmpw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\libnxsl\Release\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\libnxsl\Release\libnxsl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\nxscript\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\server\tools\nxconfig\Release\nxconfig.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server websrv
Source: "nxconfig.exe.manifest"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server websrv
; Server files
Source: "..\..\server\libnxsrv\Release\libnxsrv.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\libnxsrv\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\core\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\core\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\netxmsd\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\netxmsd\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\dbdrv\mysql\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\dbdrv\mysql\Release\mysql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\dbdrv\mssql\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\dbdrv\mssql\Release\mssql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\dbdrv\odbc\Release\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\dbdrv\pgsql\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\dbdrv\sqlite\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\dbdrv\oracle\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\smsdrv\generic\Release\generic.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\smsdrv\generic\Release\generic.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\tools\nxaction\Release\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxadm\Release\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxdbmgr\Release\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxget\Release\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxget\Release\nxget.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\server\tools\nxsnmpget\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxsnmpwalk\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxsnmpset\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\server\tools\nxupload\Release\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\nxmibc\Release\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\core\Release\nxagentd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\winnt\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\winnt\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\agent\subagents\win9x\Release\win9x.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\winperf\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\winperf\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\agent\subagents\ping\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\portCheck\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\ecs\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\ecs\Release\ecs.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\agent\subagents\ups\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\odbcquery\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mssql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mysql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_oracle.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_pgsql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_sqlite.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\images\*.ico"; DestDir: "{app}\var\images"; Flags: ignoreversion; Components: server
Source: "..\..\..\images\*.png"; DestDir: "{app}\var\images"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\mibs\*.txt"; DestDir: "{app}\var\mibs"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\netxmsd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
; Console files
Source: "..\..\console\nxuilib\Release\nxuilib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\console\nxuilib\Release\nxuilib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\console\nxuilib\Release_UNICODE\nxuilibw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\console\nxuilib\Release_UNICODE\nxuilibw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\console\nxlexer\Release\nxlexer.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\console\nxlexer\Release\nxlexer.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\console\win32\Release_UNICODE\nxcon.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\console\win32\Release_UNICODE\nxcon.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\console\nxav\Release_UNICODE\nxav.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\console\nxav\Release_UNICODE\nxav.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\console\nxnotify\Release\nxnotify.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\console\nxnotify\Release\nxnotify.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\client\console\libnxmc\Release_UNICODE\libnxmc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\client\console\nxmc\Release_UNICODE\nxmc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\client\console\plugins\AlarmBrowser\Release_UNICODE\AlarmBrowser.so"; DestDir: "{app}\lib\nxmc"; Flags: ignoreversion; Components: console
Source: "..\..\client\console\plugins\ObjectBrowser\Release_UNICODE\ObjectBrowser.so"; DestDir: "{app}\lib\nxmc"; Flags: ignoreversion; Components: console
Source: "nxcon.exe.manifest"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "nxav.exe.manifest"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "nxnotify.exe.manifest"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "nxmc.exe.manifest"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
; Web server files
Source: "..\..\nxhttpd\Release\nxhttpd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\*.js"; DestDir: "{app}\var\www"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\netxms.css"; DestDir: "{app}\var\www"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\images\*.png"; DestDir: "{app}\var\www\images"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\images\buttons\normal\*.png"; DestDir: "{app}\var\www\images\buttons\normal"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\images\buttons\pressed\*.png"; DestDir: "{app}\var\www\images\buttons\pressed"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\images\ctrlpanel\*.png"; DestDir: "{app}\var\www\images\ctrlpanel"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\images\objects\*.png"; DestDir: "{app}\var\www\images\objects"; Flags: ignoreversion; Components: websrv
Source: "..\..\nxhttpd\static\images\status\*.png"; DestDir: "{app}\var\www\images\status"; Flags: ignoreversion; Components: websrv
; Third party files
Source: "Files\mfc42.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\mfc42u.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\scilexer.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "Files\ntwdblib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mssql
Source: "Files\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\libintl-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\libiconv-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\comerr32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\krb5_32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\ssleay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\oci.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "Files\orannzsbb10.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "Files\oraociicus10.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "Files\libeay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "Files\bgd.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv
Source: "Files\zlib1.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "Files\dbghelp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "Files\wxbase28u_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\wxbase28u_xml_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\wxmsw28u_adv_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\wxmsw28u_aui_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\wxmsw28u_core_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\wxmsw28u_html_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "Files\wxmsw28u_xrc_vc.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console

[Icons]
Name: "{group}\Alarm Notifier"; Filename: "{app}\bin\nxnotify.exe"; Components: console
Name: "{group}\Alarm Viewer"; Filename: "{app}\bin\nxav.exe"; Components: console
Name: "{group}\NetXMS Console"; Filename: "{app}\bin\nxcon.exe"; Components: console
Name: "{group}\Server Configuration Wizard"; Filename: "{app}\bin\nxconfig.exe"; Components: server
Name: "{group}\Server Console"; Filename: "{app}\bin\nxadm.exe"; Parameters: "-i"; Components: server
Name: "{group}\{cm:UninstallProgram,NetXMS}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\NetXMS Console"; Filename: "{app}\bin\nxcon.exe"; Tasks: desktopicon; Components: console
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\NetXMS Console"; Filename: "{app}\bin\nxcon.exe"; Tasks: quicklaunchicon; Components: console

[Dirs]
Name: "{app}\etc"
Name: "{app}\database"
Name: "{app}\var\backgrounds"
Name: "{app}\var\packages"

[Registry]
Root: HKLM; Subkey: "Software\NetXMS"; Flags: uninsdeletekeyifempty; Components: server websrv
Root: HKLM; Subkey: "Software\NetXMS\Server"; Flags: uninsdeletekey; Components: server websrv
Root: HKLM; Subkey: "Software\NetXMS\Server"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Components: server websrv
Root: HKLM; Subkey: "Software\NetXMS\Server"; ValueType: string; ValueName: "ConfigFile"; ValueData: "{app}\etc\netxmsd.conf"; Components: server

[Run]
Filename: "{app}\bin\nxmibc.exe"; Parameters: "-z -d ""{app}\var\mibs"" -o ""{app}\var\mibs\netxms.mib"""; WorkingDir: "{app}\bin"; StatusMsg: "Compiling MIB files..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxconfig.exe"; Parameters: "--create-agent-config"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent's configuration file..."; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing agent service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting agent service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxconfig.exe"; Parameters: "--configure-if-needed"; WorkingDir: "{app}\bin"; StatusMsg: "Running server configuration wizard..."; Components: server
Filename: "{app}\bin\nxdbmgr.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" upgrade"; WorkingDir: "{app}\bin"; StatusMsg: "Upgrading database..."; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "start"; WorkingDir: "{app}\bin"; StatusMsg: "Starting core service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxconfig.exe"; Parameters: "--create-nxhttpd-config {code:GetMasterServer}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating web server's configuration file..."; Components: websrv
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-c ""{app}\etc\nxhttpd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing web server service..."; Flags: runhidden; Components: websrv
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting web server service..."; Flags: runhidden; Components: websrv

[UninstallRun]
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-S"; StatusMsg: "Stopping web server service..."; RunOnceId: "StopWebService"; Flags: runhidden; Components: websrv
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling web server service..."; RunOnceId: "DelWebService"; Flags: runhidden; Components: websrv
Filename: "{app}\bin\netxmsd.exe"; Parameters: "stop"; StatusMsg: "Stopping core service..."; RunOnceId: "StopCoreService"; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "remove"; StatusMsg: "Uninstalling core service..."; RunOnceId: "DelCoreService"; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping agent service..."; RunOnceId: "StopAgentService"; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling agent service..."; RunOnceId: "DelAgentService"; Flags: runhidden; Components: server

[Code]
Var
  HttpdSettingsPage: TInputQueryWizardPage;
  flagStartConsole: Boolean;

Function InitializeSetup(): Boolean;
Var
  i, nCount : Integer;
  param : String;
Begin
  // Set default values for flags
  flagStartConsole := FALSE;

  // Parse command line parameters
  nCount := ParamCount;
  For i := 1 To nCount Do Begin
    param := ParamStr(i);

    If Pos('/RUNCONSOLE', param) = 1 Then Begin
      flagStartConsole := TRUE;
    End;
  End;
  
  Result := TRUE;
End;

Procedure DeinitializeSetup;
Var
  strExecName: String;
  iResult: Integer;
Begin
  If flagStartConsole Then Begin
    strExecName := ExpandConstant('{app}\bin\nxcon.exe');
    If FileExists(strExecName) Then
    Begin
      Exec(strExecName, '', ExpandConstant('{app}\bin'), SW_SHOW, ewNoWait, iResult);
    End;
  End;
End;

Procedure StopAllServices;
Var
  strExecName: String;
  iResult: Integer;
Begin
  strExecName := ExpandConstant('{app}\bin\netxmsd.exe');
  If FileExists(strExecName) Then
  Begin
    Exec(strExecName, 'stop', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  End;

  strExecName := ExpandConstant('{app}\bin\nxhttpd.exe');
  If FileExists(strExecName) Then
  Begin
    Exec(strExecName, '-S', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  End;

  strExecName := ExpandConstant('{app}\bin\nxagentd.exe');
  If FileExists(strExecName) Then
  Begin
    Exec(strExecName, '-S', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  End;
End;

Procedure InitializeWizard;
Begin
  HttpdSettingsPage := CreateInputQueryPage(wpSelectTasks,
    'Select Master Server', 'Where is master server for web interface?',
    'Please enter host name or IP address of NetXMS master server. NetXMS web interface you are installing will provide connectivity to that server.');
  HttpdSettingsPage.Add('NetXMS server:', False);
  HttpdSettingsPage.Values[0] := GetPreviousData('MasterServer', 'localhost');
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'MasterServer', HttpdSettingsPage.Values[0]);
End;

Function ShouldSkipPage(PageID: Integer): Boolean;
Begin
  If PageID = HttpdSettingsPage.ID Then
    Result := not IsComponentSelected('websrv')
  Else
    Result := False;
End;

Function GetMasterServer(Param: String): String;
Begin
  Result := HttpdSettingsPage.Values[0];
End;

