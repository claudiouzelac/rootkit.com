@ECHO Just a quick batch to copy over the newly compiled driver in development stages into the windows driver directory

xcopy objchk_wxp_x86\i386\cfsd.sys \windows\system32\drivers\cfsd.sys /Y

PAUSE