@ECHO OFF
setlocal enabledelayedexpansion
TITLE Aladin OTA uploader
If "%~1"=="" ( ECHO err no IP provided )  ELSE  ( set ip=%1 )
If "%~1"=="" ( PAUSE ECHO press any key to continue )
If "%~2"=="" ( ECHO using default port 443)
If "%~2"=="" ( set port=443)  ELSE  ( set port=%2  )


If "%~3"=="" ( set file= build/blind_socket.bin )  ELSE  ( set file=%3  )
If "%~3"=="" ( ECHO using default file in %file% )  ELSE  ( ECHO using file %3  )

call :TRIM ip
call :TRIM port
call :TRIM file

ECHO curl %ip%:%port% --data-binary @- %file%

curl %ip%:%port%  --http0.9 --verbos -H "token: MYPASSWORD" --data-binary @- < %file%

ECHO exiting 
PAUSE 


:TRIM
SetLocal EnableDelayedExpansion
Call :TRIMSUB %%%1%%
EndLocal & set %1=%tempvar%
GOTO :EOF

:TRIMSUB
set tempvar=%*
GOTO :EOF
