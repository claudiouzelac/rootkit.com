if exist import32.lib goto :endimp
copy %systemroot%\system32\kernel32.dll
copy %systemroot%\system32\user32.dll
copy %systemroot%\system32\advapi32.dll
copy %systemroot%\system32\ntdll.dll
implib import32.lib kernel32.dll user32.dll advapi32.dll ntdll.dll
del *.dll
:endimp