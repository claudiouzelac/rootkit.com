
Clandestine File System Driver
Copyright (C) 2005 Jason Todd


I. INTRODUCTION

Clandestine File System Driver (cfsd) is currently a filter driver that misrepresents the underlying file system contents. It dynamically attaches to system volumes based on attach method, device type, and file system. Once it has attached itself to a volume it will start to filter IRP_MJ_DIRECTORY_CONTROL calls based upon defined match criteria. 

   File Name
   File Attributes
   File Times

It then removes any matched entries from the return essentially hiding the file.


II. PURPOSE

This driver was created with the intention of providing a layer of Security for program file protection. It is not intended to be an all encompassing module that is a bulletproof solution in all cases, but rather a mechanism for use in a bigger security strategy. On a minor level it provides a semi-sophisticated way to hide files from other users on the system.


III. DEVELOPEMENT

This is an Open source GPL project intended to evolve a great deal over time and is currently in its infant stages. It was released very early in development in hopes to stimulate its growth through community feedback, and anyone interested in advancing the project. It is no where near production quality and should only be used as light reference at best in its current state. The driver has been built and developed on a XP SP2 machine so its scope of testing at this time is severely limited.


V. REVEALING

Programs such as Rootkit revealer will be able to point out any entries hidden by the driver. This is not really a problem for this driver since it is not using subterfuge of the file system to hide anything that the user "should not" know is already there. More over it is using stealth as another measure of denying access to the file rather then just hiding it.

flister can display varied results depending on how cfsd has chosen to respond to a ZwQueryDirectoryFile() request. I do believe it is possible to completely hide from a ZwQueryDirectoryFile() request but such a method is not implemented at this time.

Being able to block access to a file at interface and source level is more in line with what ultimately the driver is designed to accomplish and not just pure stealth. Under the current implementation complete stealth is impossible because a cross-view difference will always reveal the truth.


VI. USAGE

A supplied cfsd.inf will install the required registry entries for the driver to function with a right click install. No reboot is needed and the driver can then be activated/deactivated using 'net start cfsd' and 'net stop cfsd' commands. Alternatives also are using the filter manager commands 'fltmc load cfsd' and 'fltmc unload cfsd' or 'sc' commands but the above mentioned should be adequate. The match criteria is hard coded to hide the file name 'testme.txt' any where it is found for those that do not posses the ability to recompile the driver. It is also hard coded at the moment for attach method, device, and file system so if you see a refusal in the debug it is most likely because it was not defined, cfsd uses an explicit deny method for volume types and file systems attachment.  Other scenarios in the future will use the registry for match criteria and a user mode module will also provide access if chosen as a conditional compile into the driver.The cfsd.sys provided is compiled in the XP checked buidso you can watch an incredible amount of spam about the driver’s current actions.


VII. Filter Manager

In short the filter manager appears to be Microsoft’s attempt to API file system drivers for more centralized access and system control. This in turn allows the driver to be extended across patch levels, different Microsoft operating systems, and file systems. Downside of this is that the IFS version of the DDK is required to compile this driver, but I feel the upside is worth this sacrifice. Standardized calls in the form of FltXXX functions cut down the development time significantly with most of the focus being directed towards the task at hand.

A much better definition of the filter manager and its capabilities are located in the IFS DDK with other support information available from Microsoft. Win2k received filter manager in a recent UPR with a redistributable becoming available in the very near future.


VIII - Appendix

cfsd.zip
https://www.rootkit.com/vault/merlvingian/cfsd.zip

Rootkit Revealer
http://www.sysinternals.com/utilities/rootkitrevealer.html

flister
http://invisiblethings.org/tools/flister.zip

Strider GhostBuster
http://research.microsoft.com/rootkit/ 

IFS Kit
http://www.microsoft.com/whdc/devtools/ifskit/default.mspx

Filter Manager
http://www.microsoft.com/whdc/driver/filterdrv/default.mspx

Filter Manager Win2k/2003
http://support.microsoft.com/kb/894608
