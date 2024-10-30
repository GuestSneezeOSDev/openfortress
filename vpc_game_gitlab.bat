@echo off

devtools\bin\vpc.exe /ofd +game +game_shader_dx9 /mksln ofd_all_gitlab.sln /2013
if errorlevel 1 (
	pause
)