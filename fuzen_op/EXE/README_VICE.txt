VICE 2.0
Copywrite April 2004 HBGary

Author: Jamie Butler <james.butler@hbgary.com> or <butlerjr@acm.org>

VICE is a program that identifies hooks in API calls, functions, and 
function pointer tables. It has a user portion and a kernel portion.
Usually anything it detects in the kernel is a rootkit or some form of
third party software that uses "rootkit techniques". Third party
products that may be detected by VICE in the kernel are things like
personal firewalls and Host Based Intrusion Prevention Systems (HIPS)
like ZoneAlarm, Cisco Security Agent, or Blink.

Before you can begin, VICE requires the Microsoft .NET Framework for
the GUI. You can download this for free from Microsoft.

Not all hooks are necessarily rootkits as mentioned above. There are 
even Microsoft DLL's that hook other DLL's much like a rootkit would
do. Below is a list of known DLL's that hook but are not necessarily
malicious. VICE cannot say with complete confidence that these DLL's
are not trojans because a malicious program could name itself as a
legitimate Microsoft DLL name.

1.  setupapi.dll
2.  mswsock.dll
3.  sfc_os.dll
4.  adsldpc.dll
5.  advapi32.dll
6.  secur32.dll
7.  ws2_32.dll
8.  iphlpapi.dll
9.  ntdll.dll
10. kernel32.dll
11. user32.dll
12. gdi32.dll

The above list of DLL's do hook, but are probably fine. 


FALSE POSITIVES
Anytime a DLL or the kernel report to be hooked by itself this is a 
false positive. For example, if TAPI32.dll reports to be hooked by 
TAPI32.dll, this is a false positive.

FALSE NEGATIVES
VICE may not find all rootkits. Some rootkits do not hook at all. These
usually use a layered approach in the kernel or modify memory directly
such as the FU rootkit. 

Error: Overlapped I/O operation is in progress.
This occurs when you install the driver in one place and then move it later. 
VICE is meant to run with the README, ini file, driver, and program all in
the same directory. If you move VICE on the filesystem, you need to delete
this Registry key
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\vicesys 
and reboot your machine. Now, as long as all VICE's files are in the same 
directory, it should work again.