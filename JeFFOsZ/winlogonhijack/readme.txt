WINLOGON Hijack v0.3
----------------------

1. What ?
---------

This little tool intercepts winlogon logins by injecting a dll into 
the winlogon process and logs the username, password and domain to a file.

2. How to use
-------------

Copy your dll to %systemroot% or %systemroot%\system32.
run injector.exe like this:
injector.exe dllname

dllname can be just the filename (if it is in %systemroot% or system32) or
it can be a path+filename.

Once injected, the dll stays in the winlogon process until reboot.

Logins get logged in %systemroot%\system32\mspwd.dll.

3. Changes
----------

Version 0.3:

 + Injector uses ntdll.LdrLoadDll instead of kernel32.LoadLibraryA.
 + User/password/domain log is encrypted with RC4.
 + Added a tool to decrypt the logfile.
 + Fixed the fuckedup vc++ project (it created weird empty dirs).
 - Still no terminal services logins captured

Version 0.2:

 + Injector now automatically injects all "winlogon.exe" processes
 + Code should be stabler on XP
 - No terminal services logins captured
 - No password changes captured

Version 0.1:

 - Injector needs pid for injection
 + Using 'extended code overwrite method' for hooking 
   as descriped in HF's (http://hxdef.czweb.org/) hookingen.txt 
   using LDE-32 from z0mbie (http://z0mbie.host.sk)

4. Comments
-----------

Tested in WIN2K(SP3&SP4), WINXP SP1 & WIN3K, but it should work on 
NT 3.51 & NT4 also. If ya tested it on one of theze OS, lemme know 
if worked (or not). Bugs and comment are also welcome.

Have fun ;)

JeFFOsZ@1337.be
