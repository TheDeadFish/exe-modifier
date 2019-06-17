@setlocal
@call egcc.bat
@pushd build
@set CFLAGS=%CFLAGS2%
@set CXXFLAGS=%CCFLAGS2%
@%CMAKE% -DCMAKE_EXE_LINKER_FLAGS="%LFLAGS%" ..\.
@mingw32-make
@popd

@if exist %PROGRAMS%\progs\exe_mod (
@  copy /Y bin\exe_mod.exe %PROGRAMS%\progs\exe_mod
@  copy /Y bin\libmisc32.a %PROGRAMS%\progs\exe_mod
@  copy /Y bin\libmisc64.a %PROGRAMS%\progs\exe_mod
@  copy /Y bin\exe_mod.cfg %PROGRAMS%\progs\exe_mod
@  copy /Y bin\setup.vbs %PROGRAMS%\progs\exe_mod
)
