\masm32\bin\ml /c /coff kzwenum.asm
\masm32\bin\link /driver /base:0x10000 /align:32 /out:kzwenum.sys /subsystem:native /ignore:4078 kzwenum.obj
pause
del kzwenum.obj
