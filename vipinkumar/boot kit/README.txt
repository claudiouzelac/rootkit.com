;Authors: Nitin Kumar & Vipin Kumar
;NOTE:- We are not respomsible for anything.Use at your own risk
;If you develop anything using this code, please remember to give necessary credit to the authors


just copy the NASM assembler, & use the command
nasmw.exe -f bin bootkit.asm


This is the basic version.

It has several features such as

1) It's very small.The basic framework is just about 100 lines of assembly code.It supports 2000,XP,2003
2) It patches the kernel at runtime(no files are patched on disk).(basic version has this code removed , so as others could understand it easily).
3) BOOT KIT is PXE-compatible.
4) It can even lead to first ever PXE virus
5)It also enables you to load other root kits if you have physical access(Normally root kits can only be loaded by the administrator)


