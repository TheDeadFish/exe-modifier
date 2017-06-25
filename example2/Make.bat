call exmod.bat
..\bin\cc_data happiness_island.mod music.cpp musicData
gcc *.cc %CFLAGS2% -c
exe_mod.exe calc.exe music_calc.exe calc.def *.o -lwinmm libxmp-coremod.a -lstdshit
del *.o music.cpp
