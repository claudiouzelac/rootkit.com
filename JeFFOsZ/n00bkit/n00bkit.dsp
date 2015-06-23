# Microsoft Developer Studio Project File - Name="n00bkit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=n00bkit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "n00bkit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "n00bkit.mak" CFG="n00bkit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "n00bkit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "n00bkit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "n00bkit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x813 /d "NDEBUG"
# ADD RSC /l 0x813 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "n00bkit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x813 /d "_DEBUG"
# ADD RSC /l 0x813 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "n00bkit - Win32 Release"
# Name "n00bkit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\config.c
# End Source File
# Begin Source File

SOURCE=.\engine.c
# End Source File
# Begin Source File

SOURCE=.\enumservicegroupw.c
# End Source File
# Begin Source File

SOURCE=.\enumservicesstatusa.c
# End Source File
# Begin Source File

SOURCE=.\enumservicesstatusexa.c
# End Source File
# Begin Source File

SOURCE=.\enumservicesstatusexw.c
# End Source File
# Begin Source File

SOURCE=.\ldrloaddll.c
# End Source File
# Begin Source File

SOURCE=.\ldrunloaddll.c
# End Source File
# Begin Source File

SOURCE=.\lsalogonuser.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\n00bkit.c
# End Source File
# Begin Source File

SOURCE=.\ntcreatefile.c
# End Source File
# Begin Source File

SOURCE=.\ntdeviceiocontrolfile.c
# End Source File
# Begin Source File

SOURCE=.\ntenumeratekey.c
# End Source File
# Begin Source File

SOURCE=.\ntenumeratevaluekey.c
# End Source File
# Begin Source File

SOURCE=.\ntopenfile.c
# End Source File
# Begin Source File

SOURCE=.\ntopenprocess.c
# End Source File
# Begin Source File

SOURCE=.\ntquerydirectoryfile.c
# End Source File
# Begin Source File

SOURCE=.\ntquerysysteminformation.c
# End Source File
# Begin Source File

SOURCE=.\ntqueryvirtualmemory.c
# End Source File
# Begin Source File

SOURCE=.\ntqueryvolumeinformationfile.c
# End Source File
# Begin Source File

SOURCE=.\ntreadfile.c
# End Source File
# Begin Source File

SOURCE=.\ntreadvirtualmemory.c
# End Source File
# Begin Source File

SOURCE=.\ntresumethread.c
# End Source File
# Begin Source File

SOURCE=.\ntsavekey.c
# End Source File
# Begin Source File

SOURCE=.\ntsavemergedkeys.c
# End Source File
# Begin Source File

SOURCE=.\ntvdmcontrol.c
# End Source File
# Begin Source File

SOURCE=.\recv.c
# End Source File
# Begin Source File

SOURCE=.\regraw.c
# End Source File
# Begin Source File

SOURCE=.\safe.c
# End Source File
# Begin Source File

SOURCE=.\ssl_read.c
# End Source File
# Begin Source File

SOURCE=.\wsarecv.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=.\enumservicegroupw.h
# End Source File
# Begin Source File

SOURCE=.\enumservicesstatusa.h
# End Source File
# Begin Source File

SOURCE=.\enumservicesstatusexa.h
# End Source File
# Begin Source File

SOURCE=.\enumservicesstatusexw.h
# End Source File
# Begin Source File

SOURCE=.\fileraw.h
# End Source File
# Begin Source File

SOURCE=.\ldrloaddll.h
# End Source File
# Begin Source File

SOURCE=.\ldrunloaddll.h
# End Source File
# Begin Source File

SOURCE=.\lsalogonuser.h
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\n00bk1t.h
# End Source File
# Begin Source File

SOURCE=.\ntcreatefile.h
# End Source File
# Begin Source File

SOURCE=.\ntdeviceiocontrolfile.h
# End Source File
# Begin Source File

SOURCE=.\ntenumeratekey.h
# End Source File
# Begin Source File

SOURCE=.\ntenumeratevaluekey.h
# End Source File
# Begin Source File

SOURCE=.\ntopenfile.h
# End Source File
# Begin Source File

SOURCE=.\ntopenprocess.h
# End Source File
# Begin Source File

SOURCE=.\ntquerydirectoryfile.h
# End Source File
# Begin Source File

SOURCE=.\ntquerysysteminformation.h
# End Source File
# Begin Source File

SOURCE=.\ntqueryvirtualmemory.h
# End Source File
# Begin Source File

SOURCE=.\ntqueryvolumeinformationfile.h
# End Source File
# Begin Source File

SOURCE=.\ntreadfile.h
# End Source File
# Begin Source File

SOURCE=.\ntreadvirtualmemory.h
# End Source File
# Begin Source File

SOURCE=.\ntresumethread.h
# End Source File
# Begin Source File

SOURCE=.\ntsavekey.h
# End Source File
# Begin Source File

SOURCE=.\ntsavemergedkeys.h
# End Source File
# Begin Source File

SOURCE=.\ntvdmcontrol.h
# End Source File
# Begin Source File

SOURCE=.\randoma.h
# End Source File
# Begin Source File

SOURCE=.\recv.h
# End Source File
# Begin Source File

SOURCE=.\regraw.h
# End Source File
# Begin Source File

SOURCE=.\safe.h
# End Source File
# Begin Source File

SOURCE=.\ssl_read.h
# End Source File
# Begin Source File

SOURCE=.\wsarecv.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\config.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\LDE32.OBJ
# End Source File
# End Target
# End Project
