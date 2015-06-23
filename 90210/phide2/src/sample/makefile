OUTDIR=.\Release


ALL : "$(OUTDIR)\hidecmd.sys"


"$(OUTDIR)" :
    @if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

CC=cl.exe
CC_FLAGS=-nologo -I. -I"$(DDKROOT)\inc" -I"$(DDKROOT)\inc\ddk" /Fo"$(OUTDIR)\\" -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0500 -DWINVER=0x0500 -D_WIN32_IE=0x0501 -DWIN32_LEAN_AND_MEAN=1 -DDEVL=1 -DFPO=1 -DNDEBUG -D_DLL=1 /c /Zel /Zp8 -cbstring /W3 /Gz /QIfdiv- /QIf /QI0f /GB /Gi- /Gm- /GX- /GR- /GF /Oxs /Oy -FI"$(DDKROOT)\inc\warning.h"

.c{$(OUTDIR)}.obj::
   $(CC) $(CC_FLAGS) $< 


LINK32=link.exe
LINK32_FLAGS=$(DDKROOT)\libfre\i386\ntoskrnl.lib $(DDKROOT)\libfre\i386\hal.lib /nologo /base:"0x10000" /version:5.0 /stack:0x40000,0x1000 /entry:"DriverEntry@8" /pdb:none /machine:IX86 /out:"$(OUTDIR)\hidecmd.sys" -OPT:ICF -FULLBUILD -FORCE:MULTIPLE /release -osversion:5.00 -optidata -driver -align:0x20 -subsystem:native,5.00 
LINK32_OBJS= \
	"$(OUTDIR)\hidecmd.obj" \

"$(OUTDIR)\hidecmd.sys" : "$(OUTDIR)" $(LINK32_OBJS)
    $(LINK32) $(LINK32_OBJS) $(LINK32_FLAGS)

"$(OUTDIR)\hidecmd.obj" : hidecmd.c
