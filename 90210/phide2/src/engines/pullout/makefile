OUTDIR=.\Release


ALL : "$(OUTDIR)\pullout.lib"


"$(OUTDIR)" :
   @if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

CC=cl.exe
CC_FLAGS=-nologo -I. -I"$(DDKROOT)\inc" -I"$(DDKROOT)\inc\ddk" /Fo"$(OUTDIR)\\" -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0500 -DWINVER=0x0500 -D_WIN32_IE=0x0501 -DWIN32_LEAN_AND_MEAN=1 -DDEVL=1 -DFPO=1 -DNDEBUG -D_DLL=1 /c /Zel /Zp8 -cbstring /W3 /Gz /QIfdiv- /QIf /QI0f /GB /Gi- /Gm- /GX- /GR- /GF /Oxs /Oy -FI"$(DDKROOT)\inc\warning.h"

.c{$(OUTDIR)}.obj::
   $(CC) $(CC_FLAGS) $<


LIB=lib.exe
LIB_FLAGS=/nologo /convert 
LIB_OBJS= "$(OUTDIR)\pullout.obj"


"$(OUTDIR)\pullout.lib" : "$(OUTDIR)" $(LIB_OBJS)
   $(LIB) $(LIB_FLAGS) $(LIB_OBJS)

"$(OUTDIR)\pullout.obj" : pullout.c
