program OpPorts;
{$R OpPorts.res}
{$APPTYPE CONSOLE}
uses Windows,WinSvc;
const
 DriverName='ReadMemDriver';
 DriverFileName='readmem.sys';
 IOCTL_READ_OBJ_INFO=$00222000;

 SystemProcessesAndThreadsInformation=5;
 SystemHandleInformation=16;
 ObjectNameInformation=1;
 ProcessBasicInformation=0;

 STATUS_INFO_LENGTH_MISMATCH=$C0000004;

 WINSOCK_VERSION=$0202;
 WSADESCRIPTION_LEN=256;
 WSASYS_STATUS_LEN=128;
 AF_INET=2;
 IPPROTO_TCP=6;
 IPPROTO_UDP=17;
 SOCK_STREAM=1;
 SOCK_DGRAM=2;
 SOCKET_ERROR=DWORD(-1);

type
 TWSAData=packed record
  Version,HighVersion:Word;
  Description:array[0..WSADESCRIPTION_LEN] of Char;
  SystemStatus:array[0..WSASYS_STATUS_LEN] of Char;
  MaxSockets,MaxUdpDg:Word;
  VendorInfo:PChar;
 end;
 PSockAddr=^TSockAddr;
 TSockAddr=packed record
  sin_family,sin_port:Word;
  sin_addr:Cardinal;
  sin_zero:array[0..7] of Char;
 end;

 PUnicodeString=^TUnicodeString;
 TUnicodeString=packed record
  Length,MaximumLength:Word;
  Buffer:Pointer;
 end;
 TAnsiString=packed record
  Length,MaximumLength:Word;
  Buffer:Pointer;
 end;

 TObjectInfo=packed record
  ObType,Alloc,Size,Res:Byte;
  Ptr1,Ptr2,Ptr3:Pointer;
  Flags:Cardinal;
 end;

 TRMDIn=record
  ObjAddr:Pointer;
 end;
 TRMDOut=record
  Info:TObjectInfo;
  Name:array[0..255] of Char;
 end;

 TVmCounters=packed record
  PeakVirtualSize,VirtualSize:Cardinal;
  PageFaultCount:ULONG;
  PeakWorkingSetSize,WorkingSetSize,QuotaPeakPagedPoolUsage,QuotaPagedPoolUsage,QuotaPeakNonPagedPoolUsage,QuotaNonPagedPoolUsage,PagefileUsage,PeakPagefileUsage:Cardinal;
 end;
 TIoCounters=packed record
  ReadOperationCount,WriteOperationCount,OtherOperationCount,ReadTransferCount,WriteTransferCount,OtherTransferCount:LARGE_INTEGER;
 end;
 TClientId=packed record
  UniqueProcess,UniqueThread:Cardinal;
 end;

 PObjectNameInformation=^TObjectNameInformation;
 TObjectNameInformation=packed record
  Name:TUnicodeString;
 end;
 PSystemHandleInformation=^TSystemHandleInformation;
 TSystemHandleInformation=packed record
  ProcessId:Cardinal;
  ObjectTypeNumber,Flags:Byte;
  Handle:Word;
  ObjectPtr:Pointer;
  GrantedAccess:Cardinal;
 end;
 PSystemHandleInformationEx=^TSystemHandleInformationEx;
 TSystemHandleInformationEx=packed record
  NumberOfEntries:Cardinal;
  Handles:array of TSystemHandleInformation;
 end;
 TSystemThreads=packed record
  KernelTime,UserTime,CreateTime:LARGE_INTEGER;
  WaitTime:Cardinal;
  StartAddress:Pointer;
  ClientId:TClientId;
  Priority,BasePriority,ContextSwitchCount,State,WaitReason:Cardinal;
 end;
 PSystemProcesses=^TSystemProcesses;
 TSystemProcesses=packed record
  NextEntryDelta,ThreadCount:Cardinal;
  Reserved1:array[0..5] of Cardinal;
  CreateTime,UserTime,KernelTime:LARGE_INTEGER;
  ProcessName:TUnicodeString;
  BasePriority,ProcessId,InheritedFromProcessId,HandleCount:Cardinal;
  Reserved2:array[0..1] of Cardinal;
  VmCounters:TVmCounters;
  IoCounters:TIoCounters;
  Threads:array of TSystemThreads;
 end;
 TTdiConnectionIn=packed record
  UserDataLength:Cardinal;
  UserData:Pointer;
  OptionsLength:Cardinal;
  Options:Pointer;
  RemoteAddressLength:Cardinal;
  RemoteAddress:Pointer;
 end;
 TTdiConnectionOut=packed record
  State,Event,TransmittedTsdus,ReceivedTsdus,TransmissionErrors,ReceiveErrors:Cardinal;
  Throughput,Delay:LARGE_INTEGER;
  SendBufferSize,ReceiveBufferSize,Unreliable:Cardinal;
  Unk1:array[0..5] of Cardinal;
  Unk2:Word;
 end;
 TProcessBasicInformation=packed record
  ExitStatus:Cardinal;
  PebBaseAddress:Pointer;
  AffinityMask,BasePriority,UniqueProcessId,InheritedFromUniqueProcessId:Cardinal;
 end;
 PMibUdpExRow=^TMibUdpExRow;
 TMibUdpExRow=packed record
  LocalAddr,LocalPort,ProcessId:Cardinal;
 end;
 PMibUdpExTable=^TMibUdpExTable;
 TMibUdpExTable=packed record
  NumEntries:Cardinal;
  Table:array of TMibUdpExRow;
 end;
 PMibTcpExRow=^TMibTcpExRow;
 TMibTcpExRow=packed record
  State,LocalAddr,LocalPort,RemoteAddr,RemotePort,ProcessId:Cardinal;
 end;
 PMibTcpExTable=^TMibTcpExTable;
 TMibTcpExTable=packed record
  NumEntries:Cardinal;
  Table:array of TMibTcpExRow;
 end;
 TResult=packed record
  Active:Boolean;
  Count:Integer;
  Objects:PSystemHandleInformation;
 end;

 TNtQuerySystemInformation=function(ASystemInformationClass:Cardinal;ASystemInformation:Pointer;ASystemInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtQueryObject=function(AObjectHandle:THandle;AObjectInformationClass:Cardinal;AObjectInformation:Pointer;AObjectInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TNtQueryInformationProcess=function(AProcessHandle:THandle;AProcessInformationClass:Cardinal;AProcessInformation:Pointer;AProcessInformationLength:Cardinal;AReturnLength:PCardinal):Cardinal; stdcall;
 TRtlUnicodeStringToAnsiString=function(ADestinationString:PAnsiString;ASourceString:PUnicodeString;AAllocateDestinationString:Boolean):Cardinal; stdcall;
 TRtlFreeAnsiString=function(AAnsiString:PAnsiString):Cardinal; stdcall;
 TWSAStartup=function(AVersionRequired:Word;var VWSData:TWSAData):Integer;stdcall;
 TWSACleanup=function:Integer;stdcall;
 TWSASocket=function(AFamily,AType,AProto:Integer;AProtocolInfo:Pointer;AGroup,AFlags:Cardinal):Integer;stdcall;
 Tbind=function(ASocket:Cardinal;AName:PSockAddr;ANameLen:Integer):Cardinal;stdcall;
 Tclosesocket=function(ASocket:Cardinal):Cardinal;stdcall;

 TAllocateAndGetTcpExTableFromStack=function(ATcpExTable:PMibTcpExTable;AOrder:Boolean;AHeap:THandle;AZero:Cardinal;AFlags:Cardinal):Cardinal;stdcall;
 TAllocateAndGetUdpExTableFromStack=function(AUdpExTable:PMibUdpExTable;AOrder:Boolean;AHeap:THandle;AZero:Cardinal;AFlags:Cardinal):Cardinal;stdcall;
var
 I,J:Integer;
 HandleTableSize,ProcessInfoTableSize,Status,BytesRet:Cardinal;
 HandleTable:PSystemHandleInformationEx;
 ProcessInfoTable:PSystemProcesses;
 PHandleInfo,PObj:PSystemHandleInformation;
 HandleInfo,LastObj:TSystemHandleInformation;
 SockHandleType,Proto:Byte;
 ProcessHandle,DupHandle,DrvHandle:THandle;
 TdiConnIn:TTdiConnectionIn;
 TdiConnOut:TTdiConnectionOut;
 DriverBin,ResLn,Str1:string;
 Port:Word;
 IpHlpSupport:Boolean;
 TCPPortsTable:PMibTcpExTable;
 UDPPortsTable:PMibUdpExTable;
 TCPRow:PMibTcpExRow;
 UDPRow:PMibUdpExRow;
 SockObjInfoTCP,SockObjInfoUDP,ObjInfo:TObjectInfo;
 LocPID:Cardinal;
 ResultPorts:array[0..1,0..65535] of TResult;

 NtQuerySystemInformation:TNtQuerySystemInformation;
 NtQueryObject:TNtQueryObject;
 NtQueryInformationProcess:TNtQueryInformationProcess;
 RtlUnicodeStringToAnsiString:TRtlUnicodeStringToAnsiString;
 RtlFreeAnsiString:TRtlFreeAnsiString;
 WSAStartup:TWSAStartup;
 WSACleanup:TWSACleanup;
 WSASocket:TWSASocket;
 bind:Tbind;
 closesocket:Tclosesocket;
 AllocateAndGetTcpExTableFromStack:TAllocateAndGetTcpExTableFromStack;
 AllocateAndGetUdpExTableFromStack:TAllocateAndGetUdpExTableFromStack;

procedure UninstallDriver; forward;
function DeleteFile(AFile:string):Boolean; forward;

procedure About;
begin
 WriteLn;
 WriteLn('Open Ports v1.2');
 WriteLn('programmed by Holy_Father && Ratter/29A');
 WriteLn('as a part of Hacker Defender rootkit - http://rootkit.host.sk');
 WriteLn('Copyright (c) 2000,forever ExEwORx');
 WriteLn('birthday: 29.06.2003');
 WriteLn;
end;

procedure FatalError(AErrMsg:string;AUninstDrv:Boolean=False);
begin
 WriteLn(AErrMsg);
 if AUninstDrv then
 begin
  UninstallDriver;
  DeleteFile(DriverBin);
 end;
 Halt(1);
end;

function LoadAPI:Boolean;
var
 LHMod:THandle;
begin
 LHMod:=GetModuleHandle('ntdll.dll');
 NtQuerySystemInformation:=GetProcAddress(LHMod,'NtQuerySystemInformation');
 NtQueryObject:=GetProcAddress(LHMod,'NtQueryObject');
 NtQueryInformationProcess:=GetProcAddress(LHMod,'NtQueryInformationProcess');
 RtlUnicodeStringToAnsiString:=GetProcAddress(LHMod,'RtlUnicodeStringToAnsiString');
 RtlFreeAnsiString:=GetProcAddress(LHMod,'RtlFreeAnsiString');
 LHMod:=LoadLibrary('ws2_32.dll');
 WSAStartup:=GetProcAddress(LHMod,'WSAStartup');
 WSACleanup:=GetProcAddress(LHMod,'WSACleanup');
 WSASocket:=GetProcAddress(LHMod,'WSASocketA');
 bind:=GetProcAddress(LHMod,'bind');
 closesocket:=GetProcAddress(LHMod,'closesocket');
 Result:=not ((@NtQuerySystemInformation=nil) or (@NtQueryInformationProcess=nil)
           or (@NtQueryObject=nil) or (@RtlUnicodeStringToAnsiString=nil) or (@RtlFreeAnsiString=nil)
           or (@WSAStartup=nil) or (@WSACleanup=nil) or (@WSASocket=nil) or (@bind=nil)
           or (@closesocket=nil));

 LHMod:=LoadLibrary('iphlpapi.dll');
 IpHlpSupport:=not (LHMod=0);
 if IpHlpSupport then
 begin
  AllocateAndGetTcpExTableFromStack:=GetProcAddress(LHMod,'AllocateAndGetTcpExTableFromStack');
  AllocateAndGetUdpExTableFromStack:=GetProcAddress(LHMod,'AllocateAndGetUdpExTableFromStack');
  IpHlpSupport:=not ((@AllocateAndGetTcpExTableFromStack=nil) or (@AllocateAndGetUdpExTableFromStack=nil));
 end;
end;

function GetObjInfo(AHandleInfo:TSystemHandleInformation;var VName:string):TObjectInfo;
var
 LRMDIn:TRMDIn;
 LRMDOut:TRMDOut;
 LBytesRecvd:Cardinal;
begin
 ZeroMemory(@Result,SizeOf(Result));
 LRMDIn.ObjAddr:=AHandleInfo.ObjectPtr;
 if DeviceIoControl(DrvHandle,IOCTL_READ_OBJ_INFO,@LRMDIn,SizeOf(LRMDIn),@LRMDOut,SizeOf(LRMDOut),LBytesRecvd,nil) then
 begin
  VName:=LRMDOut.Name;
  Result:=LRMDOut.Info;
 end;
end;

function ntohs(APort:Word):Word; assembler;
asm
 xchg ah,al
end;

procedure GetHandleTableAndSocketType;
var
 LWSAData:TWSAData;
 LSockTCP,LSockUDP,LPID:Cardinal;
 LI,LCurCount:Integer;
 LPHandleInfo,LPHandleInfoCur:PSystemHandleInformation;
 LAddr:TSockAddr;
 LStr:string;
 LSockFoundTCP,LSockFoundUDP:Boolean;
 LSockInfo:TObjectInfo;
begin
 WSAStartup(WINSOCK_VERSION,LWSAData);
 for LI:=1 to 65535 do
 begin
  LSockTCP:=WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,nil,0,0);
  LAddr.sin_family:=AF_INET;
  LAddr.sin_port:=ntohs(LI);
  LAddr.sin_addr:=0;
  if bind(LSockTCP,@LAddr,SizeOf(LAddr))<>SOCKET_ERROR then Break;
  closesocket(LSockTCP);
 end;
 if LAddr.sin_port=65535 then FatalError('Unable to listen.',True);
 for LI:=1 to 65535 do
 begin
  LSockUDP:=WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP,nil,0,0);
  LAddr.sin_family:=AF_INET;
  LAddr.sin_port:=ntohs(LI);
  LAddr.sin_addr:=0;
  if bind(LSockUDP,@LAddr,SizeOf(LAddr))<>SOCKET_ERROR then Break;
  closesocket(LSockUDP);
 end;
 if LAddr.sin_port=65535 then FatalError('Unable to listen.',True);

 SockHandleType:=0;
 HandleTable:=nil;
 HandleTableSize:=$10000;

 while HandleTable=nil do
 begin
  HandleTable:=Pointer(LocalAlloc(LMEM_FIXED,HandleTableSize));
  Status:=NtQuerySystemInformation(SystemHandleInformation,HandleTable,HandleTableSize,nil);
  if Status=STATUS_INFO_LENGTH_MISMATCH then
  begin
   LocalFree(Cardinal(HandleTable));
   HandleTable:=nil;
   HandleTableSize:=2*HandleTableSize;
  end else if Status<>0 then FatalError('Unable to get system handle information table.',True);
 end;

 LPID:=GetCurrentProcessId;
 LPHandleInfoCur:=nil;
 LCurCount:=0;
 LPHandleInfo:=@HandleTable^.Handles;
 for LI:=0 to HandleTable^.NumberOfEntries-1 do
 begin
  if LPHandleInfo^.ProcessId=LPID then
  begin
   Inc(LCurCount);
   if LPHandleInfoCur=nil then LPHandleInfoCur:=LPHandleInfo;
   if LPHandleInfo^.Handle=LSockTCP then SockHandleType:=LPHandleInfo^.ObjectTypeNumber;
  end else if LCurCount>0 then Break;
  Inc(LPHandleInfo);
 end;

 LSockFoundTCP:=False;
 LSockFoundUDP:=False;
 for LI:=0 to LCurCount-1 do
 begin
  if LPHandleInfoCur^.ObjectTypeNumber=SockHandleType then
  begin
   LSockInfo:=GetObjInfo(LPHandleInfoCur^,LStr);
   if (LSockInfo.Flags=1) or (LSockInfo.Flags=2) then
    if not LSockFoundTCP and (LStr='Tcp') then
    begin
     SockObjInfoTCP:=LSockInfo;
     LSockFoundTCP:=True;
    end else
    if not LSockFoundUDP and (LStr='Udp') then
    begin
     SockObjInfoUDP:=LSockInfo;
     LSockFoundUDP:=True;
    end;
   if LSockFoundTCP and LSockFoundUDP then Break;
  end;
  Inc(LPHandleInfoCur);
 end;

 closesocket(LSockTCP);
 closesocket(LSockUDP);
 WSACleanup;
 FreeLibrary(GetModuleHandle('ws2_32.dll'));
 if SockHandleType=0 then FatalError('Unable to get socket handle type.',True);
 if not (LSockFoundTCP and LSockFoundUDP) then FatalError('Unable to get socket info.',True);
end;

function GetProcessNameByPID(APID:Cardinal):string;
var
 LPProcInfo:PSystemProcesses;
 LAnsiString:TAnsiString;
 LBuf:array[0..255] of Char;

begin
 if APID<>0 then
 begin
  Result:='';
  LPProcInfo:=ProcessInfoTable;
  while (LPProcInfo^.NextEntryDelta>0) and (LPProcInfo^.ProcessId<>APID) do
   LPProcInfo:=Pointer(Cardinal(LPProcInfo)+LPProcInfo^.NextEntryDelta);
  if LPProcInfo^.ProcessId=APID then
  begin
   RtlUnicodeStringToAnsiString(@LAnsiString,@LPProcInfo^.ProcessName,True);
   CopyMemory(@LBuf,LAnsiString.Buffer,LAnsiString.Length);
   PByte(Cardinal(@LBuf)+LAnsiString.Length)^:=0;
   Result:=LBuf;
   RtlFreeAnsiString(@LAnsiString);
  end;
 end else Result:='Idle';
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

function DeleteFile(AFile:string):Boolean;
begin
 SetFileAttributes(PChar(AFile),0);
 Result:=Windows.DeleteFile(PChar(AFile));
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

function IntToStr(AInt:Integer):string;
begin
 Str(AInt,Result);
end;

function UpCase(AStr:string):string;
var
 LI:Integer;
begin
 Result:=AStr;
 for LI:=1 to Length(Result) do Result[LI]:=System.UpCase(Result[LI]);
end;

function Name2PID(AName:string):Cardinal;
var
 LPProcess:PSystemProcesses;
 LAnsiString:TAnsiString;
 LRes:Cardinal;

begin
 LRes:=$FFFFFFFF;
 AName:=UpCase(AName);
 if AName<>'IDLE' then
 begin
  LPProcess:=ProcessInfoTable;
  while (LPProcess^.NextEntryDelta>0) and (LRes=$FFFFFFFF) do
  begin
   LPProcess:=Pointer(Cardinal(LPProcess)+LPProcess^.NextEntryDelta);

   RtlUnicodeStringToAnsiString(@LAnsiString,@LPProcess^.ProcessName,True);
   if UpCase(PChar(LAnsiString.Buffer))=AName then LRes:=LPProcess^.ProcessId;
   RtlFreeAnsiString(@LAnsiString);
  end;
  Result:=LRes;
 end else Result:=0;
end;

procedure AddPort(AInfo:TSystemHandleInformation;APort:Word;AProto:Byte);
var
 LI:Integer;
 LChange:Boolean;
 LNewObj:Pointer;
 LSystemPID:Cardinal;
 LPObj:PSystemHandleInformation;
begin
 if AInfo.ProcessId=GetCurrentProcessId then Exit;
 LSystemPID:=Name2PID('System');
 with ResultPorts[AProto,APort] do
 begin
  Active:=True;
  LChange:=True;
  LPObj:=Objects;
  for LI:=0 to Count-1 do
  begin
   if (LPObj^.ProcessId=AInfo.ProcessId)
    or ((LPObj^.ObjectPtr<>nil) and (AInfo.ProcessId=LSystemPID) and (LPObj^.ObjectPtr=AInfo.ObjectPtr)) then
    begin
     LChange:=False;
     Break;
    end;
   if (LPObj^.ObjectPtr<>nil) and (LPObj^.ProcessId=LSystemPID) then
   begin
    LPObj^:=AInfo;
    LChange:=False;
    Break;
   end; 
   Inc(LPObj);
  end;
  if LChange then
  begin
   Inc(Count);
   LNewObj:=Pointer(LocalAlloc(LMEM_FIXED,Count*SizeOf(TSystemHandleInformation)));
   if Count>1 then
   begin
    CopyMemory(LNewObj,Objects,(Count-1)*SizeOf(TSystemHandleInformation));
    LocalFree(Cardinal(Objects));
   end;
   PSystemHandleInformation(Cardinal(LNewObj)+Cardinal(Count-1)*SizeOf(TSystemHandleInformation))^:=AInfo;     //sorry for this :)
   Objects:=LNewObj;
  end;
 end;
end;

begin
 About;
 if not LoadAPI then FatalError('Unable to load API.');
 if not IpHlpSupport then
 begin
  InstallAndStartDriver;
  OpenDriver;
  GetHandleTableAndSocketType;
 end;
 EnableDebugPrivilege;
 ZeroMemory(@ResultPorts,SizeOf(ResultPorts));

 ProcessInfoTable:=nil;
 ProcessInfoTableSize:=$10000;
 while ProcessInfoTable=nil do
 begin
  ProcessInfoTable:=Pointer(LocalAlloc(LMEM_FIXED,ProcessInfoTableSize));
  Status:=NtQuerySystemInformation(SystemProcessesAndThreadsInformation,ProcessInfoTable,ProcessInfoTableSize,nil);
  if Status=STATUS_INFO_LENGTH_MISMATCH then
  begin
   LocalFree(Cardinal(ProcessInfoTable));
   ProcessInfoTable:=nil;
   ProcessInfoTableSize:=2*ProcessInfoTableSize;
  end else if Status<>0 then FatalError('Unable to get system process information table.',True);
 end;

 if IpHlpSupport then
 begin
  if AllocateAndGetTcpExTableFromStack(@TCPPortsTable,True,GetProcessHeap,0,2)=0 then
  begin
   TCPRow:=@TCPPortsTable^.Table;
   for I:=0 to TCPPortsTable^.NumEntries-1 do
   begin
    ZeroMemory(@HandleInfo,SizeOf(HandleInfo));
    HandleInfo.ProcessId:=TCPRow^.ProcessId;
    AddPort(HandleInfo,ntohs(TCPRow^.LocalPort),0);
    Inc(TCPRow);
   end;
  end;
  if AllocateAndGetUdpExTableFromStack(@UDPPortsTable,False,GetProcessHeap,0,2)=0 then
  begin
   UDPRow:=@UDPPortsTable^.Table;
   for I:=0 to UDPPortsTable^.NumEntries-1 do
   begin
    ZeroMemory(@HandleInfo,SizeOf(HandleInfo));
    HandleInfo.ProcessId:=UDPRow^.ProcessId;
    AddPort(HandleInfo,ntohs(UDPRow^.LocalPort),1);
    Inc(UDPRow);
   end;
  end;
 end else
 begin
  LocPID:=GetCurrentProcessId;
  ZeroMemory(@LastObj,SizeOf(LastObj));
  LastObj.ProcessId:=$FFFFFFFF;
  ProcessHandle:=INVALID_HANDLE_VALUE;
  PHandleInfo:=@HandleTable^.Handles;
  for I:=0 to HandleTable^.NumberOfEntries-1 do
  begin
   if PHandleInfo^.ObjectTypeNumber=SockHandleType then
   begin
    if LastObj.ProcessId<>PHandleInfo^.ProcessId then
    begin
     if ProcessHandle<>INVALID_HANDLE_VALUE then CloseHandle(ProcessHandle);
     ProcessHandle:=OpenProcess(PROCESS_DUP_HANDLE,False,PHandleInfo^.ProcessId);
     LastObj:=PHandleInfo^;
     if ProcessHandle=0 then ProcessHandle:=INVALID_HANDLE_VALUE;
    end;
    if (ProcessHandle<>INVALID_HANDLE_VALUE) and (PHandleInfo^.ProcessId<>LocPID) then
    begin
     if DuplicateHandle(ProcessHandle,PHandleInfo^.Handle,GetCurrentProcess,@DupHandle,0,False,DUPLICATE_SAME_ACCESS) then
     begin
      ObjInfo:=GetObjInfo(PHandleInfo^,Str1);

      Proto:=0;
      if (ObjInfo.Flags=1) or (ObjInfo.Flags=2) then
       if ObjInfo.Ptr1=SockObjInfoTCP.Ptr1 then Proto:=1
       else if ObjInfo.Ptr1=SockObjInfoUDP.Ptr1 then Proto:=2;

      if Proto>0 then
      begin
       ZeroMemory(@TdiConnIn,SizeOf(TdiConnIn));
       ZeroMemory(@TdiConnOut,SizeOf(TdiConnOut));
       if ObjInfo.Flags=2 then
       begin
        TdiConnIn.RemoteAddressLength:=4;
        if DeviceIoControl(DupHandle,$00210012,@TdiConnIn,SizeOf(TdiConnIn),@TdiConnOut,SizeOf(TdiConnOut)-$16,BytesRet,nil) then
        begin
         TdiConnIn.RemoteAddressLength:=3;
         if DeviceIoControl(DupHandle,$00210012,@TdiConnIn,SizeOf(TdiConnIn),@TdiConnOut,SizeOf(TdiConnOut),BytesRet,nil) then
         begin
          Port:=ntohs(TdiConnOut.ReceivedTsdus);
          AddPort(LastObj,Port,Proto-1);
         end;
        end;
       end else
       begin
        TdiConnIn.RemoteAddressLength:=3;
        if DeviceIoControl(DupHandle,$00210012,@TdiConnIn,SizeOf(TdiConnIn),@TdiConnOut,SizeOf(TdiConnOut),BytesRet,nil) then
        begin
         Port:=ntohs(TdiConnOut.ReceivedTsdus);
         AddPort(LastObj,Port,Proto-1);
        end;
       end;
      end;
      CloseHandle(DupHandle);
     end;
    end;
   end;
   Inc(PHandleInfo);
  end;
  CloseHandle(ProcessHandle);
  LocalFree(Cardinal(HandleTable));
  CloseHandle(DrvHandle);
  UninstallDriver;
  DeleteFile(DriverBin);
 end;
 WriteLn('PID   Process name       Port  Proto  Process image path');
 for I:=0 to 65535 do
 with ResultPorts[0,I] do
 if Active then
 begin
  PObj:=Objects;
  for J:=0 to Count-1 do
  begin
   ResLn:=StringOfChar(' ',31)+'TCP    '+GetProcessPathByPID(PObj^.ProcessId);
   Str1:=GetProcessNameByPID(PObj^.ProcessId);
   if Length(Str1)=0 then
   begin
    Str1:='System';
    PObj^.ProcessId:=Name2PID(Str1);
   end;
   CopyMemory(@ResLn[7],@Str1[1],Length(Str1));

   Str1:=IntToStr(PObj^.ProcessId);
   CopyMemory(@ResLn[1],@Str1[1],Length(Str1));
   Str1:=IntToStr(I);
   CopyMemory(@ResLn[26],@Str1[1],Length(Str1));
   WriteLn(ResLn);
   Inc(PObj);
  end;
  if Objects<>nil then LocalFree(Cardinal(Objects));
 end;
 WriteLn;
 for I:=0 to 65535 do
 with ResultPorts[1,I] do
 if Active then
 begin
  PObj:=Objects;
  for J:=0 to Count-1 do
  begin
   ResLn:=StringOfChar(' ',31)+'UDP    '+GetProcessPathByPID(PObj^.ProcessId);
   Str1:=GetProcessNameByPID(PObj^.ProcessId);
   if Length(Str1)=0 then
   begin
    Str1:='System';
    PObj^.ProcessId:=Name2PID(Str1);
   end;
   CopyMemory(@ResLn[7],@Str1[1],Length(Str1));

   Str1:=IntToStr(PObj^.ProcessId);
   CopyMemory(@ResLn[1],@Str1[1],Length(Str1));
   Str1:=IntToStr(I);
   CopyMemory(@ResLn[26],@Str1[1],Length(Str1));
   WriteLn(ResLn);
   Inc(PObj);
  end;
  if Objects<>nil then LocalFree(Cardinal(Objects));
 end;
 LocalFree(Cardinal(ProcessInfoTable));
end.
