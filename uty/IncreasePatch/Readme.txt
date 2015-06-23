------------------------------------------------------------------------------
NIAPSoft Patch Update System V1.0
by NIAP Software					www.niapsoft.com
------------------------------------------------------------------------------

1. Patch Update System
--------------------------


Patch Update System Contains two parts:

IncreasePatch.exe generate patch file for software.
eg.From Version 1.0 to Version 1.2 there will be 3 patches.
   Update1.0_1.1.pat
   Update1.0_1.2.pat
   Update1.1_1.2.pat

UnPatch.exe patch with old version files, generate new version files

2. System Requirements
----------------------
This program runs under:
- Windows NT 4.0 SP4 or later
- Windows 2000
- Windows XP, Windows XP x64
- Windows Server 2003, Windows Server 2003 x64
- Windows Vista, Windows Vista x64
- Windows 2008, Windows 2008 x64
- Windows 7, Windows 7 x64


3. Example
----------
C company want to update there software from version 1.0 to version 1.2
First, they need to generate the patch.

IncreasePatch.exe C:\Product\V1.0 C:\Product\V1.2 C:\Update\Update1.0_1.2.pat


At client, accroding the old version files, generate new files.

UnPatch.exe C:\Downloads\Update1.0_1.2.pat C:\SomeProduct\V1.0 C:\SomeProduct\Update1.2


4. Contact Info
---------------
If you have any questions, comments or suggestions, we would like to hear from
you.

Email: niapsoft@gmail.com
Web:   www.niapsoft.com