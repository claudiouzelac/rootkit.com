set SAVEDDIR=%CD%

call %BASEDIR%\bin\setenv.bat %BASEDIR% fre w2k
cd /d %SAVEDDIR%

set SAVEDDIR=

build
