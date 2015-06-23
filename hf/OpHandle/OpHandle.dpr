program OpHandle;
{$APPTYPE CONSOLE}
uses Windows,WinSvc;
{$R OpHandle.res}
const
 DriverName='HwndNameDriver';
 DriverFileName='hwndname.sys';
 IOCTL_GET_NAME_STRING=$222000;
 IOCTL_GET_TOKEN_HANDLE=$222004;
 SystemProcessesAndThreadsInformation=$05;
 SystemHandleInformation=$10;
 STANDARD_RIGHTS_REQUIRED=$000F0000;
 DIRECTORY_ALL_ACCESS=STANDARD_RIGHTS_REQUIRED or $0F;
 ObjectNameInformation=1;
 ObjectTypeInformation=2;
 ProcessBasicInformation=0;
 ThreadBasicInformation=0;
 IdleName='Idle';
 THREAD_QUERY_INFORMATION=$0040;  

type
 PUnicodeString=^TUnicodeString;
 TUnicodeString=packed record
  Length,MaximumLength:Word;
  Buffer:PWideChar;
 end;
 PAnsiString=^TAnsiString;
 TAnsiString=packed record
  Length,MaximumLength:Word;
  Buffer:PChar;
 end;

 TDIBNameString=packed record
  PID:Cardinal;
  Handle:THandle;
 end;
 TDIBTokenHandle=packed record
  Handle:THandle;
 end;
 TDOBNameString=packed record
  Status:DWORD;
  Name:TAnsiString;
 end;
 TDOBTokenHandle=packed record
  Status:DWORD;
  Handle:THandle;
 end;

 TVmCounters=packed record
  PeakVirtualSize,VirtualSize:PCardinal;
  PageFaultCount:ULONG;
  PeakWorkingSetSize,WorkingSetSize,QuotaPeakPagedPoolUsage,QuotaPagedPoolUsage,
  QuotaPeakNonPagedPoolUsage,QuotaNonPagedPoolUsage,PagefileUsage,PeakPagefileUsage:PCardinal;
 end;
 TIoCounters=packed record
  ReadOperationCount,WriteOperationCount,OtherOperationCount,ReadTransferCount,
  WriteTransferCount,OtherTransferCount:Int64;
 end;

 TClientId=packed record
  UniqueProcess,UniqueThread:Cardinal;
 end;

 TSystemThreads=packed record
  KernelTime,UserTime,CreateTime:Int64;
  WaitTime:Cardinal;
  StartAddress:Pointer;
  ClientId:TClientId;
  Priority,BasePriority,ContextSwitchCountState,WaitReason:Cardinal;
 end;

 PSystemProcesses=^TSystemProcesses;
 TSystemProcesses=packed record
  NextEntryDelta,ThreadCount:Cardinal;
  Reserved1:array[0..5]of Cardinal;
  CreateTime,UserTime,KernelTime:Int64;
  ProcessName:TUnicodeString;
  BasePriority,ProcessId,InheritedFromProcessId,HandleCount:Cardinal;
  Reserved2:array[0..1] of Cardinal;
  VmCounters:TVmCounters;
  IoCounters:TIoCounters;
  Threads:array [0..0] of TSystemThreads;
 end;

 PTokenUser=^TTokenUser; 
 TTokenUser=packed record
  User:SID_AND_ATTRIBUTES;
 end;

 TSystemHandleInformation=packed record
  ProcessId:Cardinal;
  ObjectTypeNumber,Flags:Byte;
  Handle:Word;
  PObject:Pointer;
  GrantedAccess:Cardinal;
 end;
 PSystemHandleInformationBuffer=^TSystemHandleInformationBuffer;
 TSystemHandleInformationBuffer=packed record
  HandlesCount:Cardinal;
  HandleInfo:array [0..0] of TSystemHandleInformation;
 end;

 TGenericMapping=packed record
  GenericRead,GenericWrite,GenericExecute,GenericAll:Cardinal;
 end;
 
 PObjectNameInformation=^TObjectNameInformation;
 TObjectNameInformation=packed record
  Name:TUnicodeString;
 end;

 PObjectTypeInformation=^TObjectTypeInformation;
 TObjectTypeInformation=packed record
  Name:TUnicodeString;
  ObjectCount,HandleCount:Cardinal;
  Reserved1:array[0..3] of Cardinal;
  PeakObjectCount,PeakHandleCount:Cardinal;
  Reserved2:array[0..3] of Cardinal;
  InvalidAttributes:Cardinal;
  GenericMapping:TGenericMapping;
  ValidAccess:Cardinal;
  Unknown:Byte;
  MaintainHandleDatabase:Boolean;
  Reserved3:array[0..1] of Byte;
  PoolType:Cardinal;
  PagedPoolUsage,NonPagedPoolUsage:Cardinal;
 end;

 TProcessBasicInformation=packed record
  ExitStatus:Cardinal;
  PebBaseAddress:Pointer;
  AffinityMask,Priority,UniqueProcessId,InheritedFromUniqueProcessId:Cardinal;
 end;

 TThreadBasicInformation=packed record
  ExitStatus:Cardinal;
  TebBaseAddress:Pointer;
  ClientId:TClientId;
  AffinityMask,Priority,BasePriority:Cardinal;
 end;

 TNtQuerySystemInformation=function(ASystemInformationClass:Cardinal;ASystemInformation:Pointer;
                                    ASystemInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtQueryObject=function(AObjectHandle:THandle;AObjectInformationClass:Cardinal;AObjectInformation:Pointer;
                         AObjectInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtQueryInformationProcess=function(AProcessHandle:THandle;AProcessInformationClass:Cardinal;AProcessInformation:Pointer;AProcessInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtQueryInformationThread=function(AThreadHandle:THandle;AThreadInformationClass:Cardinal;AThreadInformation:Pointer;AThreadInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TRtlUnicodeStringToAnsiString=function(ADestinationString:PAnsiString;ASourceString:PUnicodeString;AAllocateDestinationString:Boolean):Cardinal; stdcall;
 TRtlFreeAnsiString=function(AAnsiString:PAnsiString):Cardinal; stdcall;

var
 DriverBin,ObjectName,ObjectTypeName,ObTypeShortName,ProcessName,SecID:string;
 DrvHandle,ProcessHandle,TokenHandle:THandle;
 I,J:Integer;
 ObjectTypesNames:array[0..255] of Pointer;
 CurrentPID,ProcHandles:Cardinal;

 ObjectNameBuf:array[0..1023] of Char;
 ProcessNameBuf:array[0..255] of Char;

 NtQuerySystemInformation:TNtQuerySystemInformation;
 NtQueryObject:TNtQueryObject;
 NtQueryInformationProcess:TNtQueryInformationProcess;
 NtQueryInformationThread:TNtQueryInformationThread;
 RtlUnicodeStringToAnsiString:TRtlUnicodeStringToAnsiString;
 RtlFreeAnsiString:TRtlFreeAnsiString;

 ProcessInfoTable:PSystemProcesses;
 ProcessHandleTable:PSystemHandleInformationBuffer;

function DeleteFile(AFile:string):Boolean;
begin
 SetFileAttributes(PChar(AFile),0);
 Result:=Windows.DeleteFile(PChar(AFile));
end;

function IntToStr(AInt:Integer):string; 
begin
 Str(AInt,Result);
end;

function IntToHex(ACard:Cardinal;ADigits:Byte):string;
const
 HexArray:array[0..15] of Char=('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F');
var
 LHex:string;
 LInt,LHInt:Cardinal;
begin
 LHex:=StringOfChar('0',ADigits);
 LInt:=ADigits;
 LHInt:=16;
 while ACard>0 do
 begin
  LHex[LInt]:=HexArray[(ACard mod LHint) mod 16];
  ACard:=ACard div 16;
  LHInt:=LHInt*16;
  if LHInt=0 then LHInt:=$FFFFFFFF;
  Dec(LInt);
 end;
 Result:=LHex;
end;

function UpCase(AStr:string):string; 
var
 LI:Integer;
begin
 Result:=AStr;
 for LI:=1 to Length(Result) do
  Result[LI]:=System.UpCase(Result[LI]);
end;

function EnableDebugPrivilege:Boolean;
var
 TokenHandle:THandle;
 DebugNameValue:TLargeInteger;
 Privileges:TOKEN_PRIVILEGES;
 RetLen:Cardinal;
begin
 Result:=False;
 if not OpenProcessToken(GetCurrentProcess,TOKEN_ADJUST_PRIVILEGES or TOKEN_QUERY,TokenHandle) then Exit;
 if not LookupPrivilegeValue(nil,'SeDebugPrivilege',DebugNameValue) then
 begin
  CloseHandle(TokenHandle);
  Exit;
 end;
 Privileges.PrivilegeCount:=1;
 Privileges.Privileges[0].Luid:=DebugNameValue;
 Privileges.Privileges[0].Attributes:=SE_PRIVILEGE_ENABLED;
 Result:=AdjustTokenPrivileges(TokenHandle,False,Privileges,SizeOf(Privileges),nil,RetLen);
 CloseHandle(TokenHandle);
end;

procedure UninstallDriver;
var
 LSCMHandle,LHandle:SC_HANDLE;
 LSvcStatus:TServiceStatus;
begin
 LSCMHandle:=OpenSCManager(nil,nil,SC_MANAGER_ALL_ACCESS);
 if LSCMHandle<>0 then
 begin
  LHandle:=OpenService(LSCMHandle,PChar(DriverName),SERVICE_ALL_ACCESS);
  if LHandle<>0 then
  begin
   ControlService(LHandle,SERVICE_CONTROL_STOP,LSvcStatus);
   DeleteService(LHandle);
   CloseServiceHandle(LHandle);
  end;
  CloseServiceHandle(LSCMHandle);
 end;
end;

procedure FatalError(AErrMsg:string;AUninstDrv:Boolean=False);
begin
 WriteLn(AErrMsg+' (error: '+IntToHex(GetLastError,8)+')');
 if AUninstDrv then
 begin
  UninstallDriver;
  DeleteFile(DriverBin);
 end;
 Halt;
end;

procedure LoadAPI;
var
 LHModule:HMODULE;
begin
 LHModule:=GetModuleHandle('ntdll.dll');
 NtQuerySystemInformation:=GetProcAddress(LHModule,'NtQuerySystemInformation');
 NtQueryObject:=GetProcAddress(LHModule,'NtQueryObject');
 NtQueryInformationProcess:=GetProcAddress(LHModule,'NtQueryInformationProcess');;
 NtQueryInformationThread:=GetProcAddress(LHModule,'NtQueryInformationThread');;
 RtlUnicodeStringToAnsiString:=GetProcAddress(LHModule,'RtlUnicodeStringToAnsiString');
 RtlFreeAnsiString:=GetProcAddress(LHModule,'RtlFreeAnsiString');
 if (@NtQuerySystemInformation=nil) or (@NtQueryObject=nil)
  or (@NtQueryInformationProcess=nil) or (@NtQueryInformationThread=nil)
  or (@RtlUnicodeStringToAnsiString=nil) or (@RtlFreeAnsiString=nil) then FatalError('unable to load dll');
end;

procedure InstallAndStartDriver;
var
 LPName,LPDisplayName,LPArgs:PChar;
 LSCMHandle,LHandle:SC_HANDLE;
 LType,LStart,LResSize,LBytesWritten:Cardinal;
 LRes,LGlobalRes,LFile:THandle;
 LResPtr:Pointer;
 LSysDir:array[0..MAX_PATH-1] of Char;

begin
 LPName:=PChar(DriverName);
 LPDisplayName:=PChar(DriverName);

 LRes:=FindResource(0,PChar(1),RT_RCDATA);
 LGlobalRes:=LoadResource(0,LRes);
 LResPtr:=LockResource(LGlobalRes);
 LResSize:=SizeofResource(0,LRes);
 GetSystemDirectory(LSysDir,SizeOf(LSysDir));
 DriverBin:=LSysDir;
 if not ((Length(DriverBin)=0) or (DriverBin[Length(DriverBin)]='\')) then DriverBin:=DriverBin+'\';
 DriverBin:=DriverBin+'drivers\'+DriverFileName;

 UninstallDriver;

 DeleteFile(DriverBin);
 LFile:=CreateFile(PChar(DriverBin),GENERIC_WRITE,0,nil,CREATE_ALWAYS,
                   FILE_ATTRIBUTE_READONLY,0);
 if (LFile=INVALID_HANDLE_VALUE) or (LFile=0)
  or (not WriteFile(LFile,LResPtr^,LResSize,LBytesWritten,nil))
   or (not (LBytesWritten=LResSize)) then Exit;
 CloseHandle(LFile);
 LType:=SERVICE_KERNEL_DRIVER;
 LStart:=SERVICE_DEMAND_START;

 LSCMHandle:=OpenSCManager(nil,nil,SC_MANAGER_ALL_ACCESS);
 if LSCMHandle=0 then Exit;
 LHandle:=CreateService(LSCMHandle,LPName,LPDisplayName,SERVICE_ALL_ACCESS,LType,LStart,
                        SERVICE_ERROR_IGNORE,PChar(DriverBin),nil,nil,nil,nil,nil);
 LPArgs:=nil;
 if LHandle<>0 then
 begin
  StartService(LHandle,0,LPArgs);
  CloseServiceHandle(LHandle);
 end;
 CloseServiceHandle(LSCMHandle);
end;

procedure OpenDriver;
begin
 DrvHandle:=CreateFile('\\.\'+DriverName,GENERIC_ALL,0,nil,OPEN_EXISTING,0,0);
 if DrvHandle=INVALID_HANDLE_VALUE then FatalError('unable to load driver');
end;

procedure About;
begin
 WriteLn;
 WriteLn('Open Handle v1.0');
 WriteLn('programmed by Holy_Father && Ratter/29A');
 WriteLn('as a part of Hacker Defender rootkit - http://rootkit.host.sk');
 WriteLn('Copyright (c) 2000,forever ExEwORx');
 WriteLn('birthday: 10.02.2003');
 WriteLn;
end;

function GetInfoTable(ATableType:Cardinal):Pointer; stdcall;
asm
  pop ebp                                       //Delphi call
 @GetInfoTable:
  push ebp
  mov ebp,esp
  sub esp,004h
  {
   -004         -       LResult:Pointer
   +008         -       ATableType:Cardinal
  }
  push esi
  push 000h
  pop dword ptr [ebp-004h]
  mov esi,000004000h
 @GetInfoTable_doublespace:
  shl esi,001h
  push esi
  push LMEM_FIXED
  call LocalAlloc

  test eax,eax
  jz @GetInfoTable_failed
  mov [ebp-004h],eax

  push 000h                                     //ReturnLength
  push esi                                      //SystemInformationLength
  push eax                                      //SystemInformation
  push dword ptr [ebp+008h]                     //SystemInformationClass 
  call NtQuerySystemInformation
  test eax,0C0000000h                           //SEVERITY_ERROR
  jz @GetInfoTable_end                   //STATUS_SUCCESS
  cmp eax,0C0000004h                            //STATUS_INFO_LENGTH_MISMATCH
  jnz @GetInfoTable_failed
  push dword ptr [ebp-004h]
  call LocalFree
  jmp @GetInfoTable_doublespace
 @GetInfoTable_failed:
  push 000h
  pop dword ptr [ebp-004h]

 @GetInfoTable_end:
  mov eax,[ebp-004h]
  pop esi
  leave
  ret 004h
end;

function GetObjectTypeName(AObject:TSystemHandleInformation):PChar;
var
 LDupHandle,LTargetProcHandle:THandle;
 LPObjectInfo:PObjectTypeInformation;
 LAnsiString:TAnsiString;
 LRet:Cardinal;
 LPB:PByte;
begin
 with AObject do
 begin
  if ObjectTypesNames[ObjectTypeNumber]=nil then
  begin
   LTargetProcHandle:=OpenProcess(PROCESS_DUP_HANDLE,False,AObject.ProcessId);
   if not ((LTargetProcHandle=INVALID_HANDLE_VALUE) or (LTargetProcHandle=0)) then
   begin
    if DuplicateHandle(LTargetProcHandle,AObject.Handle,GetCurrentProcess,@LDupHandle,PROCESS_QUERY_INFORMATION,False,0) then
    begin
     LRet:=0;
     NtQueryObject(LDupHandle,ObjectTypeInformation,nil,0,@LRet);
     LPObjectInfo:=Pointer(LocalAlloc(LMEM_FIXED,LRet));
     if NtQueryObject(LDupHandle,ObjectTypeInformation,LPObjectInfo,LRet,nil)=0 then
     begin
      RtlUnicodeStringToAnsiString(@LAnsiString,@LPObjectInfo^.Name,True);
      ObjectTypesNames[ObjectTypeNumber]:=Pointer(LocalAlloc(LMEM_FIXED,LAnsiString.Length+1));
      LPB:=ObjectTypesNames[ObjectTypeNumber];
      CopyMemory(LPB,LAnsiString.Buffer,LAnsiString.Length);
      Inc(LPB,LAnsiString.Length);
      LPB^:=0;
      RtlFreeAnsiString(@LAnsiString);
     end;
     LocalFree(Cardinal(LPObjectInfo));
    end;
    CloseHandle(LDupHandle);
   end;
   CloseHandle(LTargetProcHandle);
  end;
  Result:=ObjectTypesNames[ObjectTypeNumber];
 end;
end;

function GetObjectName(AObject:TSystemHandleInformation;AName:Pointer;ASize:Cardinal):Boolean;
var
 LPID,LBytesRecvd,LRet:Cardinal;
 LInBuffer:TDIBNameString;
 LOutBuffer:TDOBNameString;
 LBuffer:array[0..4095] of Char;
 LPObjectInfo:PObjectNameInformation;
 LAnsiString:TAnsiString;
 LPB:PByte;

begin
 Result:=False;
 LPID:=GetCurrentProcessId;
 with AObject do
 begin
  if ProcessId=LPID then
  begin
   LRet:=0;
   NtQueryObject(AObject.Handle,ObjectNameInformation,nil,0,@LRet);
   LPObjectInfo:=Pointer(LocalAlloc(LMEM_FIXED,LRet));

   if NtQueryObject(AObject.Handle,ObjectNameInformation,LPObjectInfo,LRet,nil)=0 then
   begin
    RtlUnicodeStringToAnsiString(@LAnsiString,@LPObjectInfo^.Name,True);
    if ASize>LAnsiString.Length then
    begin
     LPB:=AName;
     CopyMemory(LPB,LAnsiString.Buffer,LAnsiString.Length);
     Inc(LPB,LAnsiString.Length);
     LPB^:=0;
     Result:=True;
    end;
    RtlFreeAnsiString(@LAnsiString);
   end;
   LocalFree(Cardinal(LPObjectInfo));
  end else
  begin
   LInBuffer.PID:=ProcessId;
   LInBuffer.Handle:=Handle;
   if DeviceIoControl(DrvHandle,IOCTL_GET_NAME_STRING,@LInBuffer,SizeOf(LInBuffer),@LBuffer,SizeOf(LBuffer),LBytesRecvd,nil) then
   begin
    CopyMemory(@LOutBuffer,@LBuffer,SizeOf(LOutBuffer)-4);
    if Boolean(LOutBuffer.Status) then
    begin
     LOutBuffer.Name.Buffer:=Pointer(Cardinal(@LBuffer)+SizeOf(LOutBuffer));
     if ASize>=LOutBuffer.Name.Length then
     begin
      CopyMemory(AName,LOutBuffer.Name.Buffer,LOutBuffer.Name.Length);
      Result:=True;
     end;
    end; 
   end;
  end;
 end;
end;

function GetProcessInfo(APID:Cardinal;AProcHandles:PCardinal;AName:Pointer;ASize:Cardinal):Boolean;
var
 LPProcInfo:PSystemProcesses;
 LAnsiString:TAnsiString;
 LPB:PByte;
 LHandleCount,LI:Integer;

begin
 Result:=False;
 LPProcInfo:=ProcessInfoTable;
 while (LPProcInfo^.NextEntryDelta>0) and (LPProcInfo^.ProcessId<>APID) do
  LPProcInfo:=Pointer(Cardinal(LPProcInfo)+LPProcInfo^.NextEntryDelta);
 if LPProcInfo^.ProcessId=APID then
 begin
  LPB:=AName;
  if APID=0 then
  begin
   if AProcHandles<>nil then
   begin
    LHandleCount:=0;
    with ProcessHandleTable^ do
     for LI:=0 to HandlesCount-1 do if HandleInfo[LI].ProcessId=0 then Inc(LHandleCount);
    AProcHandles^:=LHandleCount;
   end;
   if ASize>=Length(IdleName)+1 then
   begin
    CopyMemory(LPB,@IdleName[1],Length(IdleName));
    Inc(LPB,Length(IdleName));
    LPB^:=0;
    Result:=True;
   end;
  end else
  begin
   if AProcHandles<>nil then AProcHandles^:=LPProcInfo^.HandleCount;
   RtlUnicodeStringToAnsiString(@LAnsiString,@LPProcInfo^.ProcessName,True);
   if ASize>=LAnsiString.Length+1 then
   begin
    CopyMemory(LPB,LAnsiString.Buffer,LAnsiString.Length);
    Inc(LPB,LAnsiString.Length);
    LPB^:=0;
    Result:=True;
   end;
   RtlFreeAnsiString(@LAnsiString);
  end;
 end;
end;

function GetTokenHandle(AProcHandle:THandle):THandle;
var
 LInBuffer:TDIBTokenHandle;
 LOutBuffer:TDOBTokenHandle;
 LBytesRecvd:Cardinal;
begin
 Result:=INVALID_HANDLE_VALUE;
 LInBuffer.Handle:=AProcHandle;
 if DeviceIoControl(DrvHandle,IOCTL_GET_TOKEN_HANDLE,@LInBuffer,SizeOf(LInBuffer),@LOutBuffer,SizeOf(LOutBuffer),LBytesRecvd,nil) then
 begin
  if Boolean(LOutBuffer.Status) then Result:=LOutBuffer.Handle;
 end;
end;

function GetSecID(ATokenHandle:THandle):string;
var
 PTokenInfo:PTokenUser;
 LRet,LRet2,LSize,LSize2:Cardinal;
 LUser,LDomain:array[0..255] of Char;
 LUserStr,LDomainStr:string;
 NameUse:SID_NAME_USE;
begin
 GetTokenInformation(ATokenHandle,TokenUser,nil,0,LRet);
 PTokenInfo:=Pointer(LocalAlloc(LMEM_FIXED,LRet));
 GetTokenInformation(ATokenHandle,TokenUser,PTokenInfo,LRet,LRet2);
 LSize:=SizeOf(LUser);
 LSize2:=SizeOf(LDomain);
 ZeroMemory(@LUser,LSize);
 ZeroMemory(@LDomain,LSize2);
 if LookupAccountSid(nil,PTokenInfo^.User.Sid,@LUser,LSize,@LDomain,LSize2,NameUse)
  and not ((LSize=0) or (LSize2=0)) then
  begin
   LUserStr:=LUser;
   LDomainStr:=LDomain;
   Result:=LUserStr+'\'+LDomainStr;
  end else Result:='<unknown>';
end;

function GetProcessHandleInfo(AObject:TSystemHandleInformation):string;
var
 LTargetProcHandle,LDupHandle:THandle;
 LProcessInfo:TProcessBasicInformation;
 LPID:Cardinal;
 LBuf:array[0..255] of Char;
begin
 Result:='<unknown process>';
 with AObject do
 begin
  LTargetProcHandle:=OpenProcess(PROCESS_DUP_HANDLE,False,ProcessId);
  if not ((LTargetProcHandle=0) or (LTargetProcHandle=INVALID_HANDLE_VALUE)) then
  begin
   if DuplicateHandle(LTargetProcHandle,Handle,GetCurrentProcess,@LDupHandle,PROCESS_QUERY_INFORMATION,False,0) then
   begin
    if NtQueryInformationProcess(LDupHandle,ProcessBasicInformation,@LProcessInfo,SizeOf(LProcessInfo),nil)=0 then
    begin
     LPID:=LProcessInfo.UniqueProcessId;
     if GetProcessInfo(LPID,nil,@LBuf,SizeOf(LBuf)) then
     begin
      Result:=LBuf;
      Result:=Result+'('+IntToHex(LPID,8)+')';
     end;
    end;
    CloseHandle(LDupHandle);
   end;
   CloseHandle(LTargetProcHandle);
  end;
 end;
end;

function GetThreadHandleInfo(AObject:TSystemHandleInformation):string;
var
 LTargetProcHandle,LDupHandle:THandle;
 LThreadInfo:TThreadBasicInformation;
 LPID:Cardinal;
 LBuf:array[0..255] of Char;
begin
 Result:='<unknown process>:<unknown thread>';
 with AObject do
 begin
  LTargetProcHandle:=OpenProcess(PROCESS_DUP_HANDLE,False,ProcessId);
  if not ((LTargetProcHandle=0) or (LTargetProcHandle=INVALID_HANDLE_VALUE)) then
  begin
   if DuplicateHandle(LTargetProcHandle,Handle,GetCurrentProcess,@LDupHandle,THREAD_QUERY_INFORMATION,False,0) then
   begin
    if NtQueryInformationThread(LDupHandle,ThreadBasicInformation,@LThreadInfo,SizeOf(LThreadInfo),nil)=0 then
    begin
     LPID:=LThreadInfo.ClientId.UniqueProcess;
     if GetProcessInfo(LPID,nil,@LBuf,SizeOf(LBuf)) then
     begin
      Result:=LBuf;
      Result:=Result+'('+IntToHex(LPID,8)+'):('+IntToHex(LThreadInfo.ClientId.UniqueThread,8)+')';
     end else Result:='<unknown process>:('+IntToHex(LThreadInfo.ClientId.UniqueThread,8)+')';
    end;
    CloseHandle(LDupHandle);
   end;
   CloseHandle(LTargetProcHandle);
  end;
 end;
end;

begin
 About;
 LoadAPI;
 InstallAndStartDriver;
 OpenDriver;
 EnableDebugPrivilege;

 ProcessInfoTable:=GetInfoTable(SystemProcessesAndThreadsInformation);
 ProcessHandleTable:=GetInfoTable(SystemHandleInformation);

 CurrentPID:=$FFFFFFFF;
 with ProcessHandleTable^ do
 for I:=0 to HandlesCount-1 do
 begin
  if CurrentPID<>HandleInfo[I].ProcessId then
  begin
   ProcHandles:=0;
   if I<>0 then WriteLn;
   WriteLn('--------------------------------------------------------------------------------');
   CurrentPID:=HandleInfo[I].ProcessId;

   if GetProcessInfo(CurrentPID,@ProcHandles,@ProcessNameBuf,SizeOf(ProcessNameBuf)) then ProcessName:=ProcessNameBuf
   else ProcessName:='<unknown process>';
   ProcessHandle:=OpenProcess(PROCESS_QUERY_INFORMATION,False,CurrentPID);
   if not ((ProcessHandle=0) or (ProcessHandle=INVALID_HANDLE_VALUE)) then
   begin
    TokenHandle:=GetTokenHandle(ProcessHandle);
    SecID:=GetSecID(TokenHandle);
    CloseHandle(TokenHandle);
    CloseHandle(ProcessHandle);
   end else SecID:='<unknown>';
   WriteLn('PID: '+IntToHex(CurrentPID,8)+', Name: '+ProcessName+', SecID: '+SecID+', Handles: '+IntToStr(ProcHandles));
  end;
  ObjectName:='';
  SetLength(ObjectTypeName,16);
  for J:=1 to Length(ObjectTypeName) do ObjectTypeName[J]:=' ';
  ObTypeShortName:=GetObjectTypeName(HandleInfo[I]);
  if Length(ObTypeShortName)>16 then ObjectTypeName:=ObTypeShortName
  else CopyMemory(@ObjectTypeName[1],@ObTypeShortName[1],Length(ObTypeShortName));
  ZeroMemory(@ObjectNameBuf,SizeOf(ObjectNameBuf));
  if GetObjectName(HandleInfo[I],@ObjectNameBuf,SizeOf(ObjectNameBuf)) then
   ObjectName:=ObjectNameBuf;
  if UpCase(ObTypeShortName)='PROCESS' then ObjectName:=GetProcessHandleInfo(HandleInfo[I])
  else if UpCase(ObTypeShortName)='THREAD' then ObjectName:=GetThreadHandleInfo(HandleInfo[I]);
  WriteLn('  '+IntToHex(HandleInfo[I].Handle,4)+': '+ObjectTypeName+ObjectName);
 end;

 LocalFree(Cardinal(ProcessInfoTable));
 LocalFree(Cardinal(ProcessHandleTable));
 CloseHandle(DrvHandle);
 UninstallDriver;
 DeleteFile(DriverBin);
end.
