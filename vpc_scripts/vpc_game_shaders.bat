@echo off

devtools\bin\vpc.exe /ofd +shaders /mksln openfortress.sln /2013

if errorlevel 1 (
	pause
)

exit