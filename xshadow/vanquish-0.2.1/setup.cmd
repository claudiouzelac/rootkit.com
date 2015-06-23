@ECHO OFF
REM Vanquish setup.

IF "%1" == "do" GOTO Command

ECHO *******************************************************************************
ECHO *                                                                             *
ECHO *                                Vanquish Setup                               *
ECHO *                                                                             *
ECHO *******************************************************************************
ECHO.
ECHO The following actions can be performed:
ECHO         help        Display ReadMe.txt file for more information.
ECHO         showlog     Display vanquish.log file from the C: root.
ECHO         install     Install a fresh copy of Vanquish from bin.
ECHO         remove      Remove Vanquish from system. Permanently.
ECHO First parameter must be 'do'. Please note that commands are case-sensitive.
ECHO.
ECHO Example: setup do remove
PAUSE>NUL

GOTO TheEnd
:Command
PUSHD
%~d0
CD %~p0
CMD /D /E:ON /C installer vanquish %2
POPD
GOTO TheEnd
:TheEnd
ECHO ON
