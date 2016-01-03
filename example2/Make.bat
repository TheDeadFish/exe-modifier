call exmod.bat
..\bin\cc_data happiness_island.mod music.cpp musicData
gcc -I..\src ..\src\stdshit.cc *.cc %CFLAGS2% -c
exe_mod.exe calc.exe music_calc.exe calc.def *.o -lwinmm libxmp-coremod.a
del *.o music.cpp
