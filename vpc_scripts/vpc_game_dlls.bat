@echo off

devtools\bin\vpc.exe /ofd +gamedlls /mksln openfortress_dlls.sln /2013

if errorlevel 1 (
	pause
)

exit