program pm;
{$APPTYPE CONSOLE}
uses Windows;
{$R pm.res}
const
 SF_BASIC       =$00000000;
 SF_HELP        =$00000001;
 SF_DETAILED    =$00000010;
 SF_TREE        =$00000020;
 SF_USE_PID     =$00000100;
 SF_KILL        =$00001000;
 SF_DOUBLE_KILL =$00002000;
 SF_MULTI_KILL  =$00004000;
 SF_ULTRA_KILL  =$00008000;
 SF_FORCE       =$00100000;

 DriverName='ProcessMasterDrv';
 DriverDeviceName='ProcessMasterDrvDevice';
 DriverFileName='pm.sys';

 IOCTL_GET_TOKEN_HANDLE=$222000;
 IOCTL_IMPERSONATE_PROCESS=$222004;
 IOCTL_KILL_PROCESS=$222008;

 SystemProcessesAndThreadsInformation=$05;
 SystemHandleInformation=$10;
 STANDARD_RIGHTS_REQUIRED=$000F0000;
 DIRECTORY_ALL_ACCESS=STANDARD_RIGHTS_REQUIRED or $0F;
 ProcessBasicInformation=0;
 ThreadBasicInformation=0;
 SystemTimeOfDayInformation=3;
 IdleName='Idle';
 THREAD_QUERY_INFORMATION=$0040;

 SERVICE_ERROR_IGNORE=$00000000;
 SERVICE_KERNEL_DRIVER=$00000001;
 SERVICE_DEMAND_START=$00000003;

 tsInitialized  =$00;
 tsReady        =$01;
 tsRunning      =$02;
 tsStandby      =$03;
 tsTerminated   =$04;
 tsWait         =$05;
 tsTransition   =$06;
 tsUnknown      =$07;

 kwrExecutive        =$00;
 kwrFreePage         =$01;
 kwrPageIn           =$02;
 kwrPoolAllocation   =$03;
 kwrDelayExecution   =$04;
 kwrSuspended        =$05;
 kwrUserRequest      =$06;
 kwrWrExecutive      =$07;
 kwrWrFreePage       =$08;
 kwrWrPageIn         =$09;
 kwrWrPoolAllocation =$0A;
 kwrWrDelayExecution =$0B;
 kwrWrSuspended      =$0C;
 kwrWrUserRequest    =$0D;
 kwrWrEventPair      =$0E;
 kwrWrQueue          =$0F;
 kwrWrLpcReceive     =$10;
 kwrWrLpcReply       =$11;
 kwrWrVirtualMemory  =$12;
 kwrWrPageOut        =$13;
 kwrWrRendezvous     =$14;
 kwrSpare2           =$15;
 kwrSpare3           =$16;
 kwrSpare4           =$17;
 kwrSpare5           =$18;
 kwrSpare6           =$19;
 kwrWrKernel         =$1A;
 kwrMaximumWaitReason=$1B;



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

 TDIBTokenHandle=packed record
  Handle:THandle;
 end;
 TDIBImpersonateProcess=packed record
  CallingProcess:THandle;
  TargetProcess:THandle;
 end;
 TDIBKillProcess=packed record
  ProcessId:Cardinal;
 end;
 TDOBTokenHandle=packed record
  Status:DWORD;
  Handle:THandle;
 end;
 TDOBImpersonateProcess=packed record
  Status1:DWORD;
  Status2:DWORD;
 end;
 TDOBKillProcess=packed record
  Status:Cardinal;
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
  KernelTime,UserTime,CreateTime,WaitTime:TFileTime;
  StartAddress:Pointer;
  ClientId:TClientId;
  Priority,BasePriority,ContextSwitchCountState,State,WaitReason:Cardinal;
 end;

 PSystemProcesses=^TSystemProcesses;
 TSystemProcesses=packed record
  NextEntryDelta,ThreadCount:Cardinal;
  Reserved1:array[0..5]of Cardinal;
  CreateTime,UserTime,KernelTime:TFileTime;
  ProcessName:TUnicodeString;
  BasePriority,ProcessId,InheritedFromProcessId,HandleCount:Cardinal;
  Reserved2:array[0..1] of Cardinal;
  VmCounters:TVmCounters;
  IoCounters:TIoCounters;
  Threads:array [0..0] of TSystemThreads;
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

 PTokenUser=^TTokenUser; 
 TTokenUser=packed record
  User:SID_AND_ATTRIBUTES;
 end;

 TProcessBasicInformation=packed record
  ExitStatus:Cardinal;
  PebBaseAddress:Pointer;
  AffinityMask,Priority,UniqueProcessId,InheritedFromUniqueProcessId:Cardinal;
 end;

 TSystemTimeInformation=packed record
  BootTime,CurrentTime,TimeZoneBias:TFileTime;
  CurrentTimeZoneID:Cardinal;
 end;

 TNtQuerySystemInformation=function(ASystemInformationClass:Cardinal;ASystemInformation:Pointer;
                                    ASystemInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtQueryInformationProcess=function(AProcessHandle:THandle;AProcessInformationClass:Cardinal;AProcessInformation:Pointer;AProcessInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtLoadDriver=function(ADriverServiceName:PUnicodeString):Cardinal; stdcall;
 TRtlAnsiStringToUnicodeString=function(ADestinationString:PUnicodeString;ASourceString:PAnsiString;AAllocateDestinationString:Boolean):Cardinal; stdcall;
 TRtlUnicodeStringToAnsiString=function(ADestinationString:PAnsiString;ASourceString:PUnicodeString;AAllocateDestinationString:Boolean):Cardinal; stdcall;
 TRtlFreeAnsiString=function(AAnsiString:PAnsiString):Cardinal; stdcall;
 TRtlFreeUnicodeString=function(AUnicodeString:PUnicodeString):Cardinal; stdcall;

var
 DriverBin,TargetName,ProcName:string;
 DrvHandle:THandle;
 StartupFlags,TargetPID,Counter:Cardinal;
 TimeInfo,TimeInfo2:TSystemTimeInformation;

 Process:PSystemProcesses;

 NtQuerySystemInformation:TNtQuerySystemInformation;
 NtLoadDriver:TNtLoadDriver;
 NtQueryInformationProcess:TNtQueryInformationProcess;
 RtlAnsiStringToUnicodeString:TRtlAnsiStringToUnicodeString;
 RtlUnicodeStringToAnsiString:TRtlUnicodeStringToAnsiString;
 RtlFreeAnsiString:TRtlFreeAnsiString;
 RtlFreeUnicodeString:TRtlFreeUnicodeString;

 ProcessInfoTable,ProcessInfoTable2:PSystemProcesses;
 ProcessHandleTable:PSystemHandleInformationBuffer;

{sysutils}
function IntToStr(AInt:Integer):string; overload;
begin
 Str(AInt,Result);
end;

function IntToStrLen(AInt:Integer;ADigits:Byte):string;
var
 LS:string;
 LI,LLen:Integer;
begin
 Result:=StringOfChar('0',ADigits);
 Str(AInt,LS);
 if Length(LS)>ADigits then LLen:=ADigits
 else LLen:=Length(LS);

 for LI:=0 to LLen-1 do
  Result[ADigits-LI]:=LS[Length(LS)-LI];
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

function ExtractFilePath(APath:string):string;
var
 LI,LJ:Integer;
begin
 if (Length(APath)<>0) and (Pos('\',APath)>0) then
 begin
  LJ:=0;
  for LI:=Length(APath) downto 1 do
   if APath[LI]='\' then
   begin
    LJ:=LI;
    Break;
   end;
  Result:=Copy(APath,1,LJ);
 end else Result:='';
end;

function ExtractFileName(APath:string):string;
var
 LI,LJ:Integer;
begin
 if Length(APath)<>0 then
 begin
  LJ:=0;
  for LI:=Length(APath) downto 1 do
   if APath[LI]='\' then
   begin
    LJ:=LI;
    Break;
   end;
  Result:=Copy(APath,LJ+1,MaxInt);
 end else Result:='';
end;

function DeleteFile(AFile:string):Boolean;
begin
 SetFileAttributes(PChar(AFile),0);
 Result:=Windows.DeleteFile(PChar(AFile));
end;

function UpCase(AStr:string):string; overload;
var
 LI:Integer;
begin
 Result:=AStr;
 for LI:=1 to Length(Result) do
  Result[LI]:=System.UpCase(Result[LI]);
end;

function StrToIntDef(AStr:string;ADef:Integer=0):Integer;
var
 LCode:Integer;
begin
 Val(AStr,Result,LCode);
 if LCode<>0 then Result:=ADef;
end;

function SystemTimeToStr(ASystemTime:TSystemTime):string;
begin
 with ASystemTime do
  Result:=IntToStrLen(wDay,2)+'.'+IntToStrLen(wMonth,2)+'.'+IntToStrLen(wYear,2)+' '+IntToStrLen(wHour,2)+':'+IntToStrLen(wMinute,2)+':'+IntToStrLen(wSecond,2);
end;


{/sysutils}

procedure UninstallDriver; forward;

function EnablePrivilege(APrivilegeName:PChar):Boolean;
var
 LTokenHandle:THandle;
 LNameValue:TLargeInteger;
 LPrivileges:TOKEN_PRIVILEGES;
 LRetLen:Cardinal;
begin
 Result:=False;
 if not OpenProcessToken(GetCurrentProcess,TOKEN_ADJUST_PRIVILEGES or TOKEN_QUERY,LTokenHandle) then Exit;
 if not LookupPrivilegeValue(nil,APrivilegeName,LNameValue) then
 begin
  CloseHandle(LTokenHandle);
  Exit;
 end;
 LPrivileges.PrivilegeCount:=1;
 LPrivileges.Privileges[0].Luid:=LNameValue;
 LPrivileges.Privileges[0].Attributes:=SE_PRIVILEGE_ENABLED;
 Result:=AdjustTokenPrivileges(LTokenHandle,False,LPrivileges,SizeOf(LPrivileges),nil,LRetLen);
 CloseHandle(LTokenHandle);
end;

function EnableDebugPrivilege:Boolean;
begin
 Result:=EnablePrivilege('SeDebugPrivilege');
end;

function EnableLoadDriverPrivilege:Boolean;
begin
 Result:=EnablePrivilege('SeLoadDriverPrivilege');
end;

procedure FatalError(AErrMsg:string);
begin
 WriteLn(AErrMsg+' (error: '+IntToHex(GetLastError,8)+')');
 Halt;
end;

function LoadAPI:Boolean;
var
 LHModule:HMODULE;
begin
 LHModule:=GetModuleHandle('ntdll.dll');
 NtQuerySystemInformation:=GetProcAddress(LHModule,'NtQuerySystemInformation');
 NtLoadDriver:=GetProcAddress(LHModule,'NtLoadDriver');
 NtQueryInformationProcess:=GetProcAddress(LHModule,'NtQueryInformationProcess');;
 RtlAnsiStringToUnicodeString:=GetProcAddress(LHModule,'RtlAnsiStringToUnicodeString');
 RtlUnicodeStringToAnsiString:=GetProcAddress(LHModule,'RtlUnicodeStringToAnsiString');
 RtlFreeAnsiString:=GetProcAddress(LHModule,'RtlFreeAnsiString');
 RtlFreeUnicodeString:=GetProcAddress(LHModule,'RtlFreeUnicodeString');
 Result:=not ((@NtQuerySystemInformation=nil) or (@NtQueryInformationProcess=nil)
          or (@NtLoadDriver=nil)
          or (@RtlAnsiStringToUnicodeString=nil) or (@RtlUnicodeStringToAnsiString=nil)
          or (@RtlFreeAnsiString=nil) or (@RtlFreeUnicodeString=nil));
end;

function GetDriverFileName(AFileName:string):string;
begin
 Result:=ExtractFilePath(ParamStr(0))+DriverFileName;
end;

function CheckRunningDriver:Boolean;
var
 LDrvHandle:THandle;
begin
 LDrvHandle:=CreateFile('\\.\'+DriverDeviceName,GENERIC_ALL,0,nil,OPEN_EXISTING,0,0);
 Result:=LDrvHandle<>INVALID_HANDLE_VALUE;
 if Result then CloseHandle(LDrvHandle);
end;

function StartDriver:Boolean;
var
 LPRegName:PChar;
 LADriverName:TAnsiString;
 LUDriverName:TUnicodeString;
begin
 Result:=CheckRunningDriver;
 if not Result then
 begin
  LPRegName:=PChar('\Registry\Machine\System\CurrentControlSet\Services\'+DriverName);
  LADriverName.Length:=Length(LPRegName);
  LADriverName.MaximumLength:=LADriverName.Length;
  LADriverName.Buffer:=LPRegName;
  RtlAnsiStringToUnicodeString(@LUDriverName,@LADriverName,True);
  NtLoadDriver(@LUDriverName);
  RtlFreeUnicodeString(@LUDriverName);
 end;

 Result:=CheckRunningDriver;
end;

procedure UninstallDriver;
begin
 RegDeleteKey(HKEY_LOCAL_MACHINE,PChar('SYSTEM\CurrentControlSet\Services\'+DriverName+'\Enum'));
 RegDeleteKey(HKEY_LOCAL_MACHINE,PChar('SYSTEM\CurrentControlSet\Services\'+DriverName+'\Security'));
 RegDeleteKey(HKEY_LOCAL_MACHINE,PChar('SYSTEM\CurrentControlSet\Services\'+DriverName));
end;

function InstallDriverAndStart:Boolean;
var
 LExec:string;
 LPName:PChar;
 LType,LStart,LResSize,LBytesWritten,LDisposition,LErrorControl:Cardinal;
 LRes,LGlobalRes,LFile:THandle;
 LResPtr:Pointer;
 LHKEY:HKEY;
begin
 Result:=CheckRunningDriver;
 if not Result then
 begin
  LPName:=PChar(DriverName);

  LRes:=FindResource(0,PChar(1),RT_RCDATA);
  LGlobalRes:=LoadResource(0,LRes);
  LResPtr:=LockResource(LGlobalRes);
  LResSize:=SizeofResource(0,LRes);
  DriverBin:=GetDriverFileName(DriverFileName);

  DeleteFile(DriverBin);
  LFile:=CreateFile(PChar(DriverBin),GENERIC_WRITE,0,nil,CREATE_ALWAYS,FILE_ATTRIBUTE_READONLY,0);
  if (LFile=INVALID_HANDLE_VALUE) or (LFile=0) or (not WriteFile(LFile,LResPtr^,LResSize,LBytesWritten,nil))
   or (not (LBytesWritten=LResSize)) then Exit;
  CloseHandle(LFile);

  LErrorControl:=SERVICE_ERROR_IGNORE;
  LExec:='\??\'+DriverBin;
  LStart:=SERVICE_DEMAND_START;
  LType:=SERVICE_KERNEL_DRIVER;

  RegCreateKeyEx(HKEY_LOCAL_MACHINE,PChar('SYSTEM\CurrentControlSet\Services\'+LPName),0,nil,
                 REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,nil,LHKEY,@LDisposition);
  RegSetValueEx(LHKEY,'ErrorControl',0,REG_DWORD,@LErrorControl,SizeOf(LErrorControl));
  RegSetValueEx(LHKEY,'ImagePath',0,REG_EXPAND_SZ,@LExec[1],Length(LExec));
  RegSetValueEx(LHKEY,'Start',0,REG_DWORD,@LStart,SizeOf(LStart));
  RegSetValueEx(LHKEY,'Type',0,REG_DWORD,@LType,SizeOf(LType));
  RegCloseKey(LHKEY);

  Result:=StartDriver;
  UninstallDriver;
  DeleteFile(DriverFileName);
 end;
end;

function OpenDriver:Boolean;
begin
 DrvHandle:=CreateFile('\\.\'+DriverDeviceName,GENERIC_ALL,0,nil,OPEN_EXISTING,0,0);
 Result:=not (DrvHandle=INVALID_HANDLE_VALUE);
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
  jz @GetInfoTable_end                          //STATUS_SUCCESS
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

function PID2Name(APID:Cardinal):string;
var
 LPProcInfo,LPProcess:PSystemProcesses;
 LAnsiString:TAnsiString;
begin
 Result:='';
 if APID<>0 then
 begin
  LPProcInfo:=GetInfoTable(SystemProcessesAndThreadsInformation);
  LPProcess:=LPProcInfo;
  while LPProcess^.NextEntryDelta>0 do
  begin
   LPProcess:=Pointer(Cardinal(LPProcess)+LPProcess^.NextEntryDelta);

   if LPProcess^.ProcessID=APID then
   begin
    RtlUnicodeStringToAnsiString(@LAnsiString,@LPProcess^.ProcessName,True);
    SetLength(Result,LAnsiString.Length);
    CopyMemory(@Result[1],LAnsiString.Buffer,LAnsiString.Length);
    RtlFreeAnsiString(@LAnsiString);
    Break;
   end;
  end;
  LocalFree(Cardinal(LPProcInfo));
 end else Result:='Idle';
end;

function Name2PID(AName:string):Cardinal;
var
 LPProcInfo,LPProcess:PSystemProcesses;
 LAnsiString:TAnsiString;
 LRes:Cardinal;

begin
 LRes:=$FFFFFFFF;
 AName:=UpCase(AName);
 if AName<>'IDLE' then
 begin
  LPProcInfo:=GetInfoTable(SystemProcessesAndThreadsInformation);
  LPProcess:=LPProcInfo;
  while (LPProcess^.NextEntryDelta>0) and (LRes=$FFFFFFFF) do
  begin
   LPProcess:=Pointer(Cardinal(LPProcess)+LPProcess^.NextEntryDelta);

   RtlUnicodeStringToAnsiString(@LAnsiString,@LPProcess^.ProcessName,True);
   if UpCase(LAnsiString.Buffer)=AName then LRes:=LPProcess^.ProcessId;
   RtlFreeAnsiString(@LAnsiString);
  end;
  LocalFree(Cardinal(LPProcInfo));
  Result:=LRes;
 end else Result:=0; 
end;

function GetProcessPathByPID(APID:Cardinal):string;
var
 LProcHandle:THandle;
 LInfo:TProcessBasicInformation;
 LProcessParametres:Pointer;
 LImagePathName:TUnicodeString;
 LPathBuf:array[0..MAX_PATH*2-1] of Char;
 LBytes:Cardinal;
 LAnsiPath:TAnsiString;
begin
 Result:='';
 ZeroMemory(@LImagePathName,SizeOf(LImagePathName));
 LProcHandle:=OpenProcess(PROCESS_VM_READ or PROCESS_QUERY_INFORMATION,False,APID);
 if (LProcHandle<>0) and (LProcHandle<>INVALID_HANDLE_VALUE) then
 begin
  if NtQueryInformationProcess(LProcHandle,ProcessBasicInformation,@LInfo,SizeOf(LInfo),nil)=0 then
  begin
   if ReadProcessMemory(LProcHandle,Pointer(Cardinal(LInfo.PebBaseAddress)+$10),@LProcessParametres,4,LBytes)
    and ReadProcessMemory(LProcHandle,Pointer(Cardinal(LProcessParametres)+$38),@LImagePathName,8,LBytes)
    and ReadProcessMemory(LProcHandle,LImagePathName.Buffer,@LPathBuf,LImagePathName.Length,LBytes) then
    begin
     LImagePathName.Buffer:=@LPathBuf;
     RtlUnicodeStringToAnsiString(@LAnsiPath,@LImagePathName,True);
     SetLength(Result,LAnsiPath.Length);
     CopyMemory(@Result[1],LAnsiPath.Buffer,LAnsiPath.Length);
     RtlFreeAnsiString(@LAnsiPath);
    end;
  end;
 end;
end;

function GetProcessCmdLineByPID(APID:Cardinal):string;
var
 LProcHandle:THandle;
 LInfo:TProcessBasicInformation;
 LProcessParametres:Pointer;
 LImagePathName:TUnicodeString;
 LPathBuf:array[0..MAX_PATH*2-1] of Char;
 LBytes:Cardinal;
 LAnsiPath:TAnsiString;
begin
 Result:='';
 ZeroMemory(@LImagePathName,SizeOf(LImagePathName));
 LProcHandle:=OpenProcess(PROCESS_VM_READ or PROCESS_QUERY_INFORMATION,False,APID);
 if (LProcHandle<>0) and (LProcHandle<>INVALID_HANDLE_VALUE) then
 begin
  if NtQueryInformationProcess(LProcHandle,ProcessBasicInformation,@LInfo,SizeOf(LInfo),nil)=0 then
  begin
   if ReadProcessMemory(LProcHandle,Pointer(Cardinal(LInfo.PebBaseAddress)+$10),@LProcessParametres,4,LBytes)
    and ReadProcessMemory(LProcHandle,Pointer(Cardinal(LProcessParametres)+$40),@LImagePathName,8,LBytes)
    and ReadProcessMemory(LProcHandle,LImagePathName.Buffer,@LPathBuf,LImagePathName.Length,LBytes) then
    begin
     LImagePathName.Buffer:=@LPathBuf;
     RtlUnicodeStringToAnsiString(@LAnsiPath,@LImagePathName,True);
     SetLength(Result,LAnsiPath.Length);
     CopyMemory(@Result[1],LAnsiPath.Buffer,LAnsiPath.Length);
     RtlFreeAnsiString(@LAnsiPath);
    end;
  end;
 end;
end;

function ImpersonateProcess(ATargetProcessHandle,ALocalProcessHandle:THandle):Cardinal;
var
 LInBuffer:TDIBImpersonateProcess;
 LOutBuffer:TDOBImpersonateProcess;
 LBytesRecvd:Cardinal;
begin
 Result:=$FFFFFFFF;
 LInBuffer.CallingProcess:=ALocalProcessHandle;
 LInBuffer.TargetProcess:=ATargetProcessHandle;
 if DeviceIoControl(DrvHandle,IOCTL_IMPERSONATE_PROCESS,@LInBuffer,SizeOf(LInBuffer),@LOutBuffer,SizeOf(LOutBuffer),LBytesRecvd,nil) then
  Result:=LOutBuffer.Status2;
end;

function ImpersonateToSystemLevel:Boolean;
var
 LSystemProcess,LLocalProcess:THandle;
 LSystemPID:Cardinal;

begin
 LSystemPID:=Name2PID('System');

 LSystemProcess:=OpenProcess(PROCESS_ALL_ACCESS,False,LSystemPID);
 LLocalProcess:=OpenProcess(PROCESS_ALL_ACCESS,False,GetCurrentProcessId);
 Result:=ImpersonateProcess(LSystemProcess,LLocalProcess)=0;
 CloseHandle(LSystemProcess);
 CloseHandle(LLocalProcess);
end;

procedure About;
begin
 WriteLn;
 WriteLn('Process Master v1.2');
 WriteLn('programmed by Holy_Father');
 WriteLn('Copyright (c) 2000,forever ExEwORx');
 WriteLn('birthday: 27.06.2003');
 WriteLn('home: http://rootkit.host.sk');
 WriteLn;
end;

procedure Usage;
var
 LExeName:string;
begin
 LExeName:=ExtractFileName(ParamStr(0));
 WriteLn('Usage: '+LExeName+' [-tk[1-4]fih?] [argument]');
 WriteLn;
 WriteLn('Run without argumets to get basic process list.');
 WriteLn('Run without flags to get detailed process info.');
 WriteLn;
 WriteLn('Flags:');
 WriteLn(' -k[lev]      - kill process');
 WriteLn('              - killing level (1=KILL,2=DOUBLE_KILL,3=MULTI_KILL,4=ULTRA_KILL)');
 WriteLn(' -f           - kill every matching process without asking');
 WriteLn(' -i           - use PID instead of the process name');
 WriteLn(' -t           - show process tree');
 WriteLn(' -?,-h        - this help');
 WriteLn;
 Halt;
end;

procedure ProcessArgs;
var
 LArg,LFlg:string;
 LI:Integer;
begin
 TargetPID:=$FFFFFFFF;
 TargetName:='';
 StartupFlags:=SF_BASIC;
 case ParamCount of
  0:StartupFlags:=SF_BASIC;
  1:begin
   LArg:=ParamStr(1);
   if (LArg<>'-?') and (LArg<>'-h') then
   begin
    if UpCase(LArg)<>'-T' then
    begin
     StartupFlags:=SF_DETAILED;
     TargetName:=UpCase(LArg);
    end else StartupFlags:=StartupFlags or SF_TREE;
   end else StartupFlags:=SF_HELP;
  end;
  2:begin
   LFlg:=UpCase(ParamStr(1));
   LArg:=ParamStr(2);
   if (Length(LFlg)>1) and (LFlg[1]='-') then
   begin
    LI:=2;
    while LI<=Length(LFlg) do
    begin
     case LFlg[LI] of
      'K':if Length(LFlg)>LI then
      begin
       Inc(LI);
       case LFlg[LI] of
        '1':StartupFlags:=StartupFlags or SF_KILL;
        '2':StartupFlags:=StartupFlags or SF_DOUBLE_KILL;
        '3':StartupFlags:=StartupFlags or SF_MULTI_KILL;
        '4':StartupFlags:=StartupFlags or SF_ULTRA_KILL;
        else begin
         StartupFlags:=StartupFlags or SF_KILL;
         Dec(LI);
        end;
       end;
      end else StartupFlags:=StartupFlags or SF_KILL;
      'I':StartupFlags:=StartupFlags or SF_USE_PID;
      'F':StartupFlags:=StartupFlags or SF_FORCE;
      else StartupFlags:=StartupFlags or SF_HELP;
     end;
     Inc(LI);
    end;
   end else StartupFlags:=SF_HELP;
   if (StartupFlags and (SF_KILL or SF_DOUBLE_KILL or SF_MULTI_KILL or SF_ULTRA_KILL)=0) then StartupFlags:=StartupFlags or SF_DETAILED;
   if StartupFlags and SF_USE_PID>0 then TargetPID:=StrToIntDef(LArg,-1)
   else TargetName:=UpCase(LArg);
   if (TargetPID=$FFFFFFFF) and (Length(TargetName)=0) then StartupFlags:=SF_HELP;
  end;
  else StartupFlags:=SF_HELP;
 end;
 if StartupFlags and SF_HELP>0 then Usage;
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
  if Boolean(LOutBuffer.Status) then Result:=LOutBuffer.Handle;
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

function GetNtCurrentTime:TSystemTimeInformation;
begin
 NtQuerySystemInformation(SystemTimeOfDayInformation,@Result,SizeOf(Result),nil);
end;

function DetailTimeToStr(ASystemTime:TSystemTime):string;
begin
 with ASystemTime do
  Result:=IntToStrLen((wDay-1)*24+wHour,2)+':'+IntToStrLen(wMinute,2)+':'+IntToStrLen(wSecond,2)+'.'+IntToStrLen(wMilliseconds,3);
end;

function RelativeSystemTime(AFT1,AFT2:TFileTime):TSystemTime;
var
 LST1,LST2:TSystemTime;
begin
 FileTimeToSystemTime(AFT1,LST1);
 FileTimeToSystemTime(AFT2,LST2);
 with LST2 do
 begin
  Dec(wYear,LST1.wYear-1601);
  Dec(wMonth,LST1.wMonth-1);
  Dec(wDay,LST1.wDay-1);
  Dec(wHour,LST1.wHour);
  Dec(wMinute,LST1.wMinute);
  Dec(wSecond,LST1.wSecond);
  Dec(wMilliseconds,LST1.wMilliseconds);
  if wMilliseconds>1000 then begin Inc(wMilliseconds,1000); Dec(wSecond); end;
  if wSecond>60 then begin Inc(wSecond,60); Dec(wMinute); end;
  if wMinute>60 then begin Inc(wMinute,60); Dec(wHour); end;
  if wHour>24 then begin Inc(wHour,24); Dec(wDay); end;
 end;
 Result:=LST2;
end;

procedure WriteProcessInfo(AProcess:PSystemProcesses);
var
 LOutLine:string[79];
 LName:string[15];
 LPID,LPPID:string[5];
 LPriority:string[2];
 LThreads:string[5];
 LHandles:string[5];
 LMemUsage:string[7];
 LRunningTime:string[14];
 LCPU:string[3];
 LStr:string;
 LAnsiString:TAnsiString;
 LI:Integer;
 LHandleCount:Cardinal;
 LDTime:TFileTime;

begin
 LOutLine:=StringOfChar(#$20,SizeOf(LOutLine));
 LName:=StringOfChar(#$20,SizeOf(LName));
 LPID:=StringOfChar(#$20,SizeOf(LPID));
 LPPID:=StringOfChar(#$20,SizeOf(LPPID));
 LPriority:=StringOfChar(#$20,SizeOf(LPriority));
 LThreads:=StringOfChar(#$20,SizeOf(LThreads));
 LHandles:=StringOfChar(#$20,SizeOf(LHandles));
 LMemUsage:=StringOfChar(#$20,SizeOf(LMemUsage));
 LRunningTime:=StringOfChar(#$20,SizeOf(LRunningTime));
 LCPU:=StringOfChar(#$20,SizeOf(LCPU));
 with AProcess^ do
 begin
  if ProcessId<>0 then
  begin
   RtlUnicodeStringToAnsiString(@LAnsiString,@ProcessName,True);
   if LAnsiString.Length>Length(LName) then LAnsiString.Length:=Length(LName);
   CopyMemory(@LName[1],LAnsiString.Buffer,LAnsiString.Length);
   RtlFreeAnsiString(@LAnsiString);
  end else LName:='Idle';

  LStr:=IntToStr(ProcessId);
  CopyMemory(@LPID[Length(LPID)-Length(LStr)+1],@LStr[1],Length(LStr));
  LStr:=IntToStr(InheritedFromProcessId);
  CopyMemory(@LPPID[Length(LPPID)-Length(LStr)+1],@LStr[1],Length(LStr));
  LStr:=IntToStr(BasePriority);
  CopyMemory(@LPriority[Length(LPriority)-Length(LStr)+1],@LStr[1],Length(LStr));
  LStr:=IntToStr(ThreadCount);
  CopyMemory(@LThreads[Length(LThreads)-Length(LStr)+1],@LStr[1],Length(LStr));
  if ProcessId=0 then
  begin
   LHandleCount:=0;
   with ProcessHandleTable^ do
    for LI:=0 to HandlesCount-1 do
    begin
     if HandleInfo[LI].ProcessId=0 then Inc(LHandleCount);
     if (LHandleCount>0) and (HandleInfo[LI].ProcessId<>0) then Break;
    end;
   LStr:=IntToStr(LHandleCount);
  end else LStr:=IntToStr(HandleCount);
  CopyMemory(@LHandles[Length(LHandles)-Length(LStr)+1],@LStr[1],Length(LStr));

  LStr:=IntToStr(Cardinal(VmCounters.WorkingSetSize) div 1024);
  CopyMemory(@LMemUsage[Length(LMemUsage)-Length(LStr)+1],@LStr[1],Length(LStr));

  if Int64(CreateTime)=0 then LDTime:=TimeInfo.BootTime
  else LDTime:=CreateTime;

  LStr:=DetailTimeToStr(RelativeSystemTime(LDTime,TimeInfo.CurrentTime));

  CopyMemory(@LRunningTime[Length(LRunningTime)-Length(LStr)+1],@LStr[1],Length(LStr));
  LStr:=IntToStrLen(Reserved1[0],3);
  CopyMemory(@LCPU[Length(LCPU)-Length(LStr)+1],@LStr[1],Length(LStr));
 end;
 CopyMemory(@LOutLine[1],@LName[1],Length(LName));
 CopyMemory(@LOutLine[21-Length(LPID)+1],@LPID[1],Length(LPID));
 CopyMemory(@LOutLine[27-Length(LPPID)+1],@LPPID[1],Length(LPPID));
 CopyMemory(@LOutLine[31-Length(LCPU)+1],@LCPU[1],Length(LCPU));
 CopyMemory(@LOutLine[40-Length(LPriority)+1],@LPriority[1],Length(LPriority));
 CopyMemory(@LOutLine[48-Length(LThreads)+1],@LThreads[1],Length(LThreads));
 CopyMemory(@LOutLine[56-Length(LHandles)+1],@LHandles[1],Length(LHandles));
 CopyMemory(@LOutLine[64-Length(LMemUsage)+1],@LMemUsage[1],Length(LMemUsage));
 CopyMemory(@LOutLine[79-Length(LRunningTime)+1],@LRunningTime[1],Length(LRunningTime));
 WriteLn(LOutLine);
end;

procedure WriteDetailedProcessInfo(AProcess:PSystemProcesses);
var
 LProcessHandle,LTokenHandle:THandle;
 LName,LPID,LPPID,LPriority,LThreads,LHandles,LRunningTime,LCreateTime,LUserTime,
 LKernelTime,LSecID,LCPU,LMemUsage,LMemUsagePeak,LVirtualMemory,LVirtualMemoryPeak,LPagefileUsage,
 LPagefileUsagePeak,LPagedPoolUsage,LPagedPoolUsagePeak,LNonPagedPoolUsage,LNonPagedPoolUsagePeak,
 LPageFaultCount,LPath,LCmdLine:string;
 LAnsiString:TAnsiString;
 LHandleCount:Cardinal;
 LI:Integer;
 LDTime:TFileTime;
 LSysTime:TSystemTime;
begin
 with AProcess^ do
 begin
  if ProcessId<>0 then
  begin
   RtlUnicodeStringToAnsiString(@LAnsiString,@ProcessName,True);
   SetLength(LName,LAnsiString.Length);
   CopyMemory(@LName[1],LAnsiString.Buffer,LAnsiString.Length);
   RtlFreeAnsiString(@LAnsiString);
  end else LName:='Idle';

  LPID:=IntToStr(ProcessId);
  LPPID:=IntToStr(InheritedFromProcessId);
  LPriority:=IntToStr(BasePriority);
  LThreads:=IntToStr(ThreadCount);
  if ProcessId=0 then
  begin
   LHandleCount:=0;
   with ProcessHandleTable^ do
    for LI:=0 to HandlesCount-1 do
    begin
     if HandleInfo[LI].ProcessId=0 then Inc(LHandleCount);
     if (LHandleCount>0) and (HandleInfo[LI].ProcessId<>0) then Break;
    end;
   LHandles:=IntToStr(LHandleCount);
  end else LHandles:=IntToStr(HandleCount);
  LMemUsage:=IntToStr(Cardinal(VmCounters.WorkingSetSize));
  LMemUsagePeak:=IntToStr(Cardinal(VmCounters.PeakWorkingSetSize));
  LVirtualMemory:=IntToStr(Cardinal(VmCounters.VirtualSize));
  LVirtualMemoryPeak:=IntToStr(Cardinal(VmCounters.PeakVirtualSize));
  LPagefileUsage:=IntToStr(Cardinal(VmCounters.PagefileUsage));
  LPagefileUsagePeak:=IntToStr(Cardinal(VmCounters.PeakPagefileUsage));
  LPagedPoolUsage:=IntToStr(Cardinal(VmCounters.QuotaPagedPoolUsage));
  LPagedPoolUsagePeak:=IntToStr(Cardinal(VmCounters.QuotaPeakPagedPoolUsage));
  LNonPagedPoolUsage:=IntToStr(Cardinal(VmCounters.QuotaNonPagedPoolUsage));
  LNonPagedPoolUsagePeak:=IntToStr(Cardinal(VmCounters.QuotaPeakNonPagedPoolUsage));
  LPageFaultCount:=IntToStr(Cardinal(VmCounters.PageFaultCount));

  if Int64(CreateTime)=0 then LDTime:=TimeInfo.BootTime
  else LDTime:=CreateTime;

  LRunningTime:=DetailTimeToStr(RelativeSystemTime(LDTime,TimeInfo.CurrentTime));
  FileTimeToSystemTime(CreateTime,LSysTime);
  LCreateTime:=SystemTimeToStr(LSysTime);
  FileTimeToSystemTime(UserTime,LSysTime);
  LUserTime:=DetailTimeToStr(LSysTime);
  FileTimeToSystemTime(KernelTime,LSysTime);
  LKernelTime:=DetailTimeToStr(LSysTime);

  LCPU:=IntToStrLen(Reserved1[0],3);
 end;

 LProcessHandle:=OpenProcess(PROCESS_QUERY_INFORMATION,False,AProcess^.ProcessId);
 if not ((LProcessHandle=0) or (LProcessHandle=INVALID_HANDLE_VALUE)) then
 begin
  LTokenHandle:=GetTokenHandle(LProcessHandle);
  LSecID:=GetSecID(LTokenHandle);
  CloseHandle(LTokenHandle);
  CloseHandle(LProcessHandle);
 end else LSecID:='<unknown>';

 LPath:=GetProcessPathByPID(AProcess^.ProcessId);
 LCmdLine:=GetProcessCmdLineByPID(AProcess^.ProcessId);

 WriteLn('Process name:              '+LName);
 WriteLn('Process path:              '+LPath);
 WriteLn('Process command line:      '+LCmdLine);
 WriteLn('Process ID:                '+LPID);
 WriteLn('Parent process ID:         '+LPPID);
 WriteLn('Current CPU usage:         '+LCPU+'%');
 WriteLn('Priority:                  '+LPriority);
 WriteLn('Number of threads:         '+LThreads);
 WriteLn('Number of handles:         '+LHandles);
 WriteLn('Running time:              '+LRunningTime);
 WriteLn('Create time:               '+LCreateTime);
 WriteLn('Kernel time:               '+LKernelTime);
 WriteLn('User time:                 '+LUserTime);
 WriteLn('Security ID:               '+LSecID);
 WriteLn('Memory usage:              '+LMemUsage);
 WriteLn('Memory usage peak:         '+LMemUsagePeak);
 WriteLn('Virtual memory:            '+LVirtualMemory);
 WriteLn('Virtual memory peak:       '+LVirtualMemoryPeak);
 WriteLn('Pagefile usage:            '+LPagefileUsage);
 WriteLn('Pagefile usage peak:       '+LPagefileUsagePeak);
 WriteLn('Paged pool usage:          '+LPagedPoolUsage);
 WriteLn('Paged pool usage peak:     '+LPagedPoolUsagePeak);
 WriteLn('Non-paged pool usage:      '+LNonPagedPoolUsage);
 WriteLn('Non-paged pool usage peak: '+LNonPagedPoolUsagePeak);
 WriteLn('Page faults:               '+LPageFaultCount);
 WriteLn;
end;

procedure WriteProcessTree(AParentProcess,AProcess:PSystemProcesses;ASpaces:Integer);
var
 LName,LPIDStr:string;
 LAnsiString:TAnsiString;
begin
 with AProcess^ do
 begin
  if ProcessId<>0 then
  begin
   RtlUnicodeStringToAnsiString(@LAnsiString,@ProcessName,True);
   SetLength(LName,LAnsiString.Length);
   CopyMemory(@LName[1],LAnsiString.Buffer,LAnsiString.Length);
   RtlFreeAnsiString(@LAnsiString);
  end else LName:='Idle';
  LPIDStr:=' ('+IntToStr(ProcessId)+')';
  WriteLn(LName:Length(LName)+ASpaces,LPIDStr:15-Length(LName)+Length(LPIDStr));
  ThreadCount:=$FFFFFFFF;

  AParentProcess:=AProcess;
  while AProcess^.NextEntryDelta<>0 do
  begin
   AProcess:=Pointer(Cardinal(AProcess)+AProcess^.NextEntryDelta);
   if (AParentProcess^.ProcessId=AProcess^.InheritedFromProcessId)
    and (Int64(AParentProcess^.CreateTime)<=Int64(AProcess^.CreateTime)) then
     WriteProcessTree(AParentProcess,AProcess,ASpaces+2);
  end;
 end;
end;

function OwnProcess(APID:Cardinal):Boolean;
begin
 Result:=APID=GetCurrentProcessId;
 if Result then WriteLn('I will not kill myself!');
end;

function ConfirmKill(APID:Cardinal;AName:string):Boolean;
var
 LAnswer:string;
begin
 if StartupFlags and SF_FORCE=0 then
 begin
  Write('Do you want to kill '+AName+' ('+IntToStr(APID)+')?: ');
  ReadLn(LAnswer);
  LAnswer:=UpCase(LAnswer);
  Result:=(LAnswer='Y') or (LAnswer='YES');
 end else Result:=True;
end;

procedure Kill(APID:Cardinal);
var
 LProcessHandle:THandle;
 LName:string;
begin
 if OwnProcess(APID) then Exit;
 LName:=PID2Name(APID);
 if not ConfirmKill(APID,LName) then Exit;
 LProcessHandle:=OpenProcess(PROCESS_TERMINATE,False,APID);
 if not ((LProcessHandle=0) or (LProcessHandle=INVALID_HANDLE_VALUE)) then
 begin
  if TerminateProcess(LProcessHandle,0) then WriteLn(LName+' ('+IntToStr(APID)+') killed')
  else WriteLn('unable to kill '+LName+' ('+IntToStr(APID)+'), error: '+IntToStr(GetLastError));
  CloseHandle(LProcessHandle);
 end else WriteLn('unable to open '+LName+' ('+IntToStr(APID)+'), error: '+IntToStr(GetLastError));
end;

procedure DoubleKill(APID:Cardinal);
begin
 EnableDebugPrivilege;
 Kill(APID);
end;

procedure MultiKill(APID:Cardinal);
begin
 EnableDebugPrivilege;
 ImpersonateToSystemLevel;
 Kill(APID);
end;

procedure UltraKill(APID:Cardinal);
var
 LInBuffer:TDIBKillProcess;
 LOutBuffer:TDOBKillProcess;
 LBytesRecvd:Cardinal;
 LName:string;
begin
 if OwnProcess(APID) then Exit;
 LName:=PID2Name(APID);
 if not ConfirmKill(APID,LName) then Exit;

 EnableDebugPrivilege;
 ImpersonateToSystemLevel;

 LInBuffer.ProcessId:=APID;
 if DeviceIoControl(DrvHandle,IOCTL_KILL_PROCESS,@LInBuffer,SizeOf(LInBuffer),@LOutBuffer,
                    SizeOf(LOutBuffer),LBytesRecvd,nil) then
 begin
  if LOutBuffer.Status=0 then WriteLn(LName+' ('+IntToStr(APID)+') killed')
  else WriteLn('unable to kill '+LName+' ('+IntToStr(APID)+'), error: 0x'+IntToHex(LOutBuffer.Status,8));
 end else WriteLn('unable to connect driver, error: '+IntToStr(GetLastError));
end;

procedure CountCPUUsage(AProcessInfoTable,AProcessInfoTable2:PSystemProcesses;ACurrentTime:TFileTime);
var
 LP1,LP2:PSystemProcesses;
 LTotalUsage,LDiv:Int64;
begin
 LTotalUsage:=0;
 LP1:=AProcessInfoTable;
 LP2:=AProcessInfoTable2;

 while (LP2^.ProcessId<>LP1^.ProcessId) do
  if LP2^.NextEntryDelta<>0 then LP2:=Pointer(Cardinal(LP2)+LP2^.NextEntryDelta)
  else Break;

 LTotalUsage:=LTotalUsage+LP1^.UserTime.dwLowDateTime-LP2^.UserTime.dwLowDateTime
                         +LP1^.KernelTime.dwLowDateTime-LP2^.KernelTime.dwLowDateTime
                         +LP1^.UserTime.dwHighDateTime-LP2^.UserTime.dwHighDateTime
                         +LP1^.KernelTime.dwHighDateTime-LP2^.KernelTime.dwHighDateTime;
 while LP1^.NextEntryDelta<>0 do
 begin
  LP1:=Pointer(Cardinal(LP1)+LP1^.NextEntryDelta);
  while (LP2^.ProcessId<>LP1^.ProcessId) do
   if LP2^.NextEntryDelta<>0 then LP2:=Pointer(Cardinal(LP2)+LP2^.NextEntryDelta)
   else Break;

  LTotalUsage:=LTotalUsage+LP1^.UserTime.dwLowDateTime-LP2^.UserTime.dwLowDateTime
                          +LP1^.KernelTime.dwLowDateTime-LP2^.KernelTime.dwLowDateTime
                          +LP1^.UserTime.dwHighDateTime-LP2^.UserTime.dwHighDateTime
                          +LP1^.KernelTime.dwHighDateTime-LP2^.KernelTime.dwHighDateTime;
 end;

 LP1:=AProcessInfoTable;
 LP2:=AProcessInfoTable2;

 while (LP2^.ProcessId<>LP1^.ProcessId) do
  if LP2^.NextEntryDelta<>0 then LP2:=Pointer(Cardinal(LP2)+LP2^.NextEntryDelta)
  else Break;

 LDiv:=LP1^.UserTime.dwLowDateTime-LP2^.UserTime.dwLowDateTime
      +LP1^.KernelTime.dwLowDateTime-LP2^.KernelTime.dwLowDateTime
      +LP1^.UserTime.dwHighDateTime-LP2^.UserTime.dwHighDateTime
      +LP1^.KernelTime.dwHighDateTime-LP2^.KernelTime.dwHighDateTime;

 if LDiv=0 then LP1^.Reserved1[0]:=0
 else LP1^.Reserved1[0]:=Trunc(LDiv/LTotalUsage*100) mod 101;
 while LP1^.NextEntryDelta<>0 do
 begin
  LP1:=Pointer(Cardinal(LP1)+LP1^.NextEntryDelta);
  while (LP2^.ProcessId<>LP1^.ProcessId) do
   if LP2^.NextEntryDelta<>0 then LP2:=Pointer(Cardinal(LP2)+LP2^.NextEntryDelta)
   else Break;

  LDiv:=LP1^.UserTime.dwLowDateTime-LP2^.UserTime.dwLowDateTime
       +LP1^.KernelTime.dwLowDateTime-LP2^.KernelTime.dwLowDateTime
       +LP1^.UserTime.dwHighDateTime-LP2^.UserTime.dwHighDateTime
       +LP1^.KernelTime.dwHighDateTime-LP2^.KernelTime.dwHighDateTime;

  if LDiv=0 then LP1^.Reserved1[0]:=0
  else LP1^.Reserved1[0]:=Trunc(LDiv/LTotalUsage*100)  mod 101;
 end;
end;


begin
 About;
 if not LoadAPI then FatalError('unable to load dll');

 ProcessInfoTable2:=GetInfoTable(SystemProcessesAndThreadsInformation);
 TimeInfo2:=GetNtCurrentTime;

 ProcessArgs;
 EnableLoadDriverPrivilege;

 if not InstallDriverAndStart then
 begin
  WriteLn('Warning: you haven''t got administrator''s privileges');
  WriteLn('         some features may be disabled');
 end else if not OpenDriver then FatalError('unable to open driver');

 Sleep(100);
 ProcessInfoTable:=GetInfoTable(SystemProcessesAndThreadsInformation);
 if ProcessInfoTable=nil then FatalError('unable to load process info');
 TimeInfo:=GetNtCurrentTime;

 TimeInfo2.CurrentTime.dwLowDateTime:=TimeInfo.CurrentTime.dwLowDateTime-TimeInfo2.CurrentTime.dwLowDateTime;
 TimeInfo2.CurrentTime.dwHighDateTime:=TimeInfo.CurrentTime.dwHighDateTime-TimeInfo2.CurrentTime.dwHighDateTime;

 ProcessHandleTable:=GetInfoTable(SystemHandleInformation);
 if ProcessHandleTable=nil then FatalError('unable to load handle table');

 CountCPUUsage(ProcessInfoTable,ProcessInfoTable2,TimeInfo2.CurrentTime);

 if StartupFlags=SF_BASIC then
 begin
  WriteLn('Name              PID  PPID CPU Priority Threads Handles  Memory   Running time');
  //       12345678911234567892123456789312345678941234567895123456789612345678971234567898

  Process:=ProcessInfoTable;

  WriteProcessInfo(Process);
  while Process^.NextEntryDelta<>0 do
  begin
   Process:=Pointer(Cardinal(Process)+Process^.NextEntryDelta);
   WriteProcessInfo(Process);
  end;
 end else
 if StartupFlags=SF_TREE then
 begin
  Process:=ProcessInfoTable;
  WriteProcessTree(Process,Process,0);
  while Process^.NextEntryDelta<>0 do
  begin
   Process:=Pointer(Cardinal(Process)+Process^.NextEntryDelta);
   if Process^.ThreadCount<>$FFFFFFFF then WriteProcessTree(Process,Process,0);
  end;
 end else
 begin
  Process:=ProcessInfoTable;
  Counter:=0;
  if TargetPID<>$FFFFFFFF then
  begin
   if Process^.ProcessId<>TargetPID then
   while (Process^.NextEntryDelta<>0) and (Process^.ProcessId<>TargetPID) do
    Process:=Pointer(Cardinal(Process)+Process^.NextEntryDelta);

   if Process^.ProcessId=TargetPID then
   begin
    if StartupFlags and SF_DETAILED>0 then WriteDetailedProcessInfo(Process)
    else if StartupFlags and SF_KILL>0 then Kill(Process^.ProcessId)
    else if StartupFlags and SF_DOUBLE_KILL>0 then DoubleKill(Process^.ProcessId)
    else if StartupFlags and SF_MULTI_KILL>0 then MultiKill(Process^.ProcessId)
    else if StartupFlags and SF_ULTRA_KILL>0 then UltraKill(Process^.ProcessId);
   end else WriteLn('Process does not exist!');
  end else
  begin
   Process:=ProcessInfoTable;
   ProcName:=UpCase(PID2Name(Process^.ProcessId));
   if ProcName=TargetName then
   begin
    if StartupFlags and SF_DETAILED>0 then WriteDetailedProcessInfo(Process)
    else if StartupFlags and SF_KILL>0 then Kill(Process^.ProcessId)
    else if StartupFlags and SF_DOUBLE_KILL>0 then DoubleKill(Process^.ProcessId)
    else if StartupFlags and SF_MULTI_KILL>0 then MultiKill(Process^.ProcessId)
    else if StartupFlags and SF_ULTRA_KILL>0 then UltraKill(Process^.ProcessId);
    Inc(Counter);
   end;
   while Process^.NextEntryDelta<>0 do
   begin
    Process:=Pointer(Cardinal(Process)+Process^.NextEntryDelta);
    ProcName:=UpCase(PID2Name(Process^.ProcessId));
    if ProcName=TargetName then
    begin
     if StartupFlags and SF_DETAILED>0 then WriteDetailedProcessInfo(Process)
     else if StartupFlags and SF_KILL>0 then Kill(Process^.ProcessId)
     else if StartupFlags and SF_DOUBLE_KILL>0 then DoubleKill(Process^.ProcessId)
     else if StartupFlags and SF_MULTI_KILL>0 then MultiKill(Process^.ProcessId)
     else if StartupFlags and SF_ULTRA_KILL>0 then UltraKill(Process^.ProcessId);
     Inc(Counter);
    end;
   end;
   if Counter=0 then WriteLn('Process does not exist!');
  end;
 end;

 LocalFree(Cardinal(ProcessInfoTable));
 LocalFree(Cardinal(ProcessHandleTable));
 CloseHandle(DrvHandle);
end.
