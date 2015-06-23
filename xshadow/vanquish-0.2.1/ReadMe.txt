*******************************************************************************
*                                                                             *
*                               Vanquish v0.2.1                               *
*                                                                             *
*******************************************************************************
           *                                                        *
           *  Copyright (c)2003-2005 XShadow. All rights reserved.  *
           *                                                        *
           **********************************************************

                            *** USE AT OWN RISK ***


What's new
====================
     WARNING THIS IS NOT AN OFFICIAL VERSION. THIS IS AN INTERMEDIATE
VERSION THAT FIXES THE WIN2K REGISTRY PROBLEMS.

Installation/Removal
====================
Simple: copy the files to a folder on your HD and then
        double-click setup.cmd and follow the instructions from there.

Please note that setup.cmd is a wrapper to installer.cmd that allows it to
detect vanquish even if it is installed. So no other commands are needed
before issuing a setup command.
Also, take great care for path entries. On my system, typing 'setup' in a
command prompt -- besides the folder with vanquish -- runs setup.exe from
c:\windows\system32 because it is in the current path.

As a side note, vanquish was designed to run with **Adminstrator** privilege.
Please, before mailing that vanquish doesn't work check that you have this
privilege, and that you installed vanquish as Administrator.
Also, vanquish cannot run concurrently with other API rewriting apps(rootkits).

System Requirements
====================
Supported OS: Microsoft Windows 2K/XP/2K3

NB: Will not function properly under 'Windows 9x/Me'. Other system based on
NT 5.0 or higher *should* work fine (e.g. Longhorn).

Did somebody test it under wine? Let me know. :)

Contact
====================
E-mail: xshadow #at# email #dot# ro
WWW:    www.rootkit.com > Vanquish Project

Description
====================
Vanquish is a DLL-Injection based rootkit that hides files, folders, registry
entries and logs passwords.

Vanquish has two different modules:
    a) Vanquish Autoloader - vanquish.exe
    b) Vanquish DLL        - vanquish.dll

a) Vanquish Autoloader
When run, vanquish.exe searches for these command-line arguments:
    -install    Add vanquish.exe to Services; inject DLL into running apps.
    -remove     Remove vanquish from Services; needs restart to remove DLL.
It is not recommended to use directly. Use setup.cmd script instead.
When no recognizable command-line argument is found, vanquish.exe defaults
to injecting the DLL into running apps and exit. Thus, you will never see
Vanquish Autoloader as 'STARTED' in the Services MMC. This is so that
no one will spot it's process in Task Manager.

b) Vanquish DLL
Vanquish DLL is internally divided into SIX submodules:

1)DllUtils
    Inject Vanquish DLL into new processes.          (CreateProcess(AsUser)A/W)
    Make sure nobody will unload Vanquish DLL.                    (FreeLibrary)

    Utilites related to dll functionality and propagation.

2)HideFiles
    Hide files/folders containing the magic string "vanquish".
                                              (FindFirstFileExW, FindNextFileW)

    A hidden file/folder won't get reported by windowze as occupying space and
    cannot be found with "Search For Files or Folders..." or similar, and a
    folder containing hidden files/folders cannot be erased.

3)HideReg
    Hide registry entries containing the same magic string.
                                  (RegCloseKey, RegEnumKeyA/W, RegEnumKeyExA/W,
                                    RegEnumValueA/W, RegQueryMultipleValuesA/W)

    Uses an advanced cached system to keep track of enumerated keys/values and
    hide the ones that need to be hidden, but keeping the high-performance of
    registry.

4)HideServices
    Hide service entries containing the magic string in their name.
	                                                    (EnumServicesStatusA/W)

    Uses the same logic as in HideReg but without the complicated caching
	system.

5)PwdLog
    Logs username, passwords and domain.
                                                (LogonUserA/W, WlxLoggedOutSAS)

    First it intercepts calls to LogonUserW (probably 'runas' commands, switch
    user and similar). To get the logon prompt password we hook the msgina dll
    function WlxLoggedOutSAS. The passwords are present in the logfile.

6)SourceProtect
    Prevent deletion of files/folders that start with D:\MY
                                            (DeleteFileA/W, RemoveDirectoryA/W)
    Prevent change of system time.
	                                 (SetLocalTime, SetTimeZoneInformation,
                                        SetSystemTimeAdjustment, SetSystemTime)

    This is a personal add-on that I use on my computer so no one will delete
    my files on a FAT partition. It is not active in the binary-version of
    Vanquish DLL.


Other features:
    Error logging - everything (including passwords) will be shown in a file
                    called 'vanquish.log' in the root of c:
                  - please don't report dll injection failed errors as they
                    are very common; I'm working on it...

    Vanquish RunTime Library - functions to replace CRT malloc(), free(),
    realloc(), memcpy(), memset(); also contains the logging functions and
    exception handling routines.

    Advanced injector - it can modify any process (even the SYSTEM ones...);
    uses the new method described in my paper "Executing arbitrary code in a
    chosen process(or advanced dll injection)" on [www.rootkit.com]; the thread
    handle, if not available is found by simulating a GetCurrentThreadId()
    using a ReadProcessMemory() call; does not use CreateRemoteThread!

    Subjective injection - a hidden executable (e.g. ...\xyvanquish.exe) or a
    normal executable in a hidden folder (e.g. ...\hiddenvanquish\xy.exe) will
    NOT get injected with vanquish.dll and so it will be able to see what
    others can not. Another example is to include "vanquish" as a parameter
    with the same effect (e.g. ...\blah\regedit.exe vanquish)

Known Problems
===================
* Native & DOS apps are not injectable (no native api is used).
* Terminal services is not injected (probably need native api).

Program history
===================
v0.2.1 ---------- fixed registry hiding for Windows 2K
                - modified injector to support unloading
                - added some pointer checks for magic string finding
                - fixed Windows XP SP2 bugs
                - updated vanquish.exe process listing
                - improved GINA hooking code (fully dynamical now)
v0.2.0 ---------- speed-up magic string finding
                - cut the CRT slack (files are now very small)
                - added VRT (Vanquish Run Time) to use instead of e.g. malloc
                - added bind utility for additional speed bonus
                - modified RKI to support dynamic(on-heap) registry entries
                - fixed lengthy stack allocations (now uses VRT)
                - non-privileged log file
                - fixed debug privilege errors (now silent but smart)
                - service hiding module added
                - setup script now very complex
                - advanced stack-tracing exception-handler
                - a smart (more relaxed) process-injector (uses what it can)
                - added module hiding facility
v0.1 beta9 ------ service implementation
                - updated injector (no longer using CreateRemoteThread)
                - dynamic api replacement
                - password logging uses WlxLoggedOutSAS
                - use FlushInstructionCache() for every API write (for MP sys)
v0.1 beta8 ------ included version information
                - unload protection added
                - code divided into modules
                - fixed CreateProcessW so it won't deadlock
                - added CreateProcessAsUserW for more comprehensive injection
                  so taskmgr.exe gets injected too!
                - registry hiding is a lot faster (forgot to update time when a
                  free slot was found and thus RKI was rebuilding the indexes
                  every time a registry read was intercepted)
                - minor coding in HideFiles module
                - password logging extended to logon prompt
v0.1 beta7 ------ extended the old error reporting
                - added RegQueryMultipleValues
                - cleaned up the code a little bit
                - new registry hiding method that WORKS!
                - fixed a nasty stack overflow bug
                - subjective injection
                - password logging
v0.1 beta6 ------ it hides itself(changed hiding string from "hidethis" to
                  "vanquish")
v0.1 beta5 ------ registry hiding added
v0.1 beta4 ------ "Add/Remove Programs" would block for INFINITY
                - added some basic error reporting
v0.1 beta3 ------ now the injection method is DIRECT (smaller and efficient)
                - the injector algorithm works on SYSTEM processes
                - code made smaller (hooked only UNICODE functions)
v0.1 beta2 ------ it affects all explorer.exe child processes
v0.1 beta  ------ it affects only explorer.exe (no child process injection)

How does it work
===================
It hooks API calls and processes them. If needed it calls the REAL Windows API
for additional processing. The injection method is described in my paper
"Executing arbitrary code in a chosen process(or advanced dll injection)" on
[www.rootkit.com]. The effective API replace is done by overwriting the first 5
bytes of the API prolog with a 32bit offset jmp instruction to our NEW Vanquish
API wich resides in the injected DLL(vanquish.dll); if needed Vanquish will
rebuild the first 5 bytes of the REAL API and call it, then write the jmp once
again. Vanquish DLL employs a number of thread-synchronization tehniques to
be safe. The only problem with this method is that lengthy functions could be
called again while being un-hooked for original api forwarding. Testing showed
that in about 4% - 5% of the cases (on a nonblocking function) calls would not
go to the NEW Vanquish API.

The Vanquish DLL is initially injected into all windowed exe images by Vanquish
Autoloader. The injection code is also present in Vanquish DLL and is activated
when a call to CreateProcess(AsUser)A/W is intercepted.

TO IMPLEMENT
===================
    - 64bit support
    - Longhorn support
    - custom options for files && registry
    - hook SetUnhandledExceptionFilter
    - user hiding
    - event log hiding
    - port hiding
    - process hiding
    - TS support

THE SOFTWARE PACKAGE
====================
includes the following files:

bin\vanquish.exe - the initial injector program
bin\vanquish.dll - this is the heart of the program
installer.cmd - installation script; do not call directly
setup.cmd - installation wrapper batch file for installer.cmd
ReadMe.txt - this file

3RD PARTY FILES
====================
bin\bind.exe - 3rd party utility * Copyright (c) Microsoft Corp. 1981-1998

This is used to bind Vanquish to your Windows release. Binding reduces loading
times incurred by Vanquish DLL.

LEGAL DISCLAIMER
===================

			     NO WARRANTY

      PLEASE NOTE THAT THE TERM "THE PROGRAM" REFERS TO THE SOFTWARE
PACKAGE DEFINED ABOVE. YOU CAN DISTRIBUTE THIS PROGRAM FREELY AS LONG AS
IT IS INTACT (IT INCLUDES ALL OF THE FILES NAMED ABOVE) AND YOU DON'T
CHARGE FOR IT OR IT'S DISTRIBUTION.

      THE SOLE EXISTENCE OF ANY FILE (TEXT OR BINARY) ON YOUR SYSTEM THAT
IS INCLUDED IN THE SOFTWARE PACKAGE IS CONSIDERED AS AN ACCEPTANCE OF THIS
DISCLAIMER EVEN IF THE PROGRAM WAS NEVER EXECUTED AND/OR COPIED BY THE
COMPUTER OWNER/ADMINISTRATOR AND EVEN IF THIS DISCLAIMER IS NOT PRESENT
OR HAS NOT BEEN READ BY THE COMPUTER OWNER/ADMINISTRATOR.

      BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

      IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.


-EOF-
