OUTDIR=.\Release


ALL : "$(OUTDIR)\catchy32.lib"


"$(OUTDIR)" :
   @if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

AS=ml.exe
AS_FLAGS=/nologo /c /coff /Fo"$(OUTDIR)\\"

.asm{$(OUTDIR)}.obj::
   $(AS) $(AS_FLAGS) $<


LIB=lib.exe
LIB_FLAGS=/nologo /convert 
LIB_OBJS= "$(OUTDIR)\catchy32.obj"


"$(OUTDIR)\catchy32.lib" : "$(OUTDIR)" $(LIB_OBJS)
   $(LIB) $(LIB_FLAGS) $(LIB_OBJS) 

"$(OUTDIR)\catchy32.obj" : catchy32.asm
