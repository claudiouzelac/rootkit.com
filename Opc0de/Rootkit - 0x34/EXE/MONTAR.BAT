ml /c /coff /Cp getvar.asm
rc resource.rc
link /SUBSYSTEM:WINDOWS /LIBPATH:c:\masm32\lib getvar.obj resource.res
pause
del *.obj