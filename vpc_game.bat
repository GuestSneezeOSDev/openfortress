@echo off

devtools\bin\vpc.exe /ofd +game +game_shader_dx9 /mksln openfortress.sln /2013

if errorlevel 1 (
	pause
)

exit