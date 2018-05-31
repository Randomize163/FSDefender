xcopy /R /Y D:\FSD C:\Users\User\Desktop\FSD\

RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultUninstall 132 C:\Users\User\Desktop\FSD\fsd.inf
RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 C:\Users\User\Desktop\FSD\fsd.inf

fltmc load fsd
fltmc attach fsd C:

C:\Users\User\Desktop\scripts\copy_FSD_manager.bat