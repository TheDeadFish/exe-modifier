call egcc.bat
gcc -I. src\*.cc src\peFile\*.cc  src\linker\*.cc %CFLAGS2% -g -c
gcc -I. src\main.cpp %CFLAGS2% -g *.o -o bin\exe_mod.exe -limagehlp 

if exist %PROGRAMS%\progs\exe_mod (
  copy /Y bin\exe_mod.exe %PROGRAMS%\progs\exe_mod
  copy /Y bin\df_link.exe %PROGRAMS%\progs\exe_mod
  copy /Y bin\libmisc.a %PROGRAMS%\progs\exe_mod
  copy /Y bin\exe_mod.cfg %PROGRAMS%\progs\exe_mod
  copy /Y bin\setup.vbs %PROGRAMS%\progs\exe_mod
  copy /Y bin\Bind.exe %PROGRAMS%\progs\exe_mod  
  copy /Y bin\bind_exe.bat %PROGRAMS%\progs\exe_mod  
)
