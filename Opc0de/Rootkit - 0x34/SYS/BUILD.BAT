del ..\exe\getvar.sys
\masm32\bin\rc getvar.rc
\masm32\bin\ml /nologo /c /coff getvar.asm
\masm32\bin\link /nologo /driver /base:0x10000 /align:32 /out:getvar.sys /subsystem:native /ignore:4078 getvar.obj getvar.res
copy getvar.sys ..\exe\
pause

