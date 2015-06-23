@ECHO OFF
VERIFY ON
REM Do not run directly.
IF NOT "%1" == "vanquish" GOTO Stop

IF "%2" == "help" GOTO Help
IF "%2" == "showlog" GOTO ShowLog
IF "%2" == "install" GOTO Install
IF "%2" == "remove" GOTO RemoveActive

ECHO Unknow command.

GOTO EndNow
:ShowLog
MORE /E /C c:\vanquish.log

GOTO EndNow
:Help
MORE /E /C .\ReadMe.txt

GOTO EndNow
:Install
ECHO *******************************************************************************
ECHO *                                                                             *
ECHO *                               Install Vanquish                              *
ECHO *                                                                             *
ECHO *******************************************************************************
ECHO.
ECHO Checking for previous version...
IF EXIST %SystemRoot%\vanquish.exe GOTO AlreadyInstalled
IF EXIST %SystemRoot%\vanquish.dll GOTO AlreadyInstalled
ECHO Vanquish not found. Fresh install.
ECHO Are you sure you want to install? Press CTRL+C now to abort.
PAUSE>NUL

ECHO Installing...
COPY .\bin\vanquish.exe %SystemRoot%\vanquish.exe
COPY .\bin\vanquish.dll %SystemRoot%\vanquish.dll
BIND -u %SystemRoot%\vanquish.exe
BIND -u %SystemRoot%\vanquish.dll
START /WAIT %SystemRoot%\vanquish.exe -install

ECHO.
ECHO.
ECHO Vanquish installed. To complete process you do NOT need to reboot.

GOTO EndNow
:AlreadyInstalled
ECHO Vanquish is already installed. Please remove first.

GOTO EndNow
:RemoveActive
ECHO *******************************************************************************
ECHO *                                                                             *
ECHO *                               Remove Vanquish                               *
ECHO *                                                                             *
ECHO *******************************************************************************
ECHO.
ECHO Checking for previous installation...
IF NOT EXIST %SystemRoot%\vanquish.exe GOTO NotInstalled
IF NOT EXIST %SystemRoot%\vanquish.dll GOTO NotInstalled
ECHO Vanquish found in %SystemRoot%
ECHO Are you sure you want to remove? Press CTRL+C now to abort.
PAUSE>NUL

ECHO Removing...
START /WAIT %SystemRoot%\vanquish.exe -remove
DEL /F /Q %SystemRoot%\vanquish.exe
DEL /F /Q %SystemRoot%\vanquish.dll

ECHO.
ECHO.
ECHO Vanquish removed. To complete process you do NOT need to reboot.

GOTO EndNow
:NotInstalled
ECHO Vanquish is not installed. Nothing to remove.
Goto EndNow
:Stop
ECHO *** Do not run directly. Use [setup.cmd] instead.
GOTO EndNow
:EndNow
ECHO Press any key to continue.
PAUSE>NUL
VERIFY OFF
ECHO ON
