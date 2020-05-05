@call cmake_gcc

@if exist %PROGRAMS%\progs\exe_mod (
@  copy /Y bin\exe_mod.exe %PROGRAMS%\progs\exe_mod
@  copy /Y bin\libmisc32.a %PROGRAMS%\progs\exe_mod
@  copy /Y bin\libmisc64.a %PROGRAMS%\progs\exe_mod
@  copy /Y bin\exe_mod.cfg %PROGRAMS%\progs\exe_mod
@  copy /Y bin\setup.vbs %PROGRAMS%\progs\exe_mod
)

:: install library
@if not exist %PROGRAMS%\local\include\peFile ( 
	mkdir %PROGRAMS%\local\include\peFile  )
@copy /Y src\peFile\*.h %PROGRAMS%\local\include\peFile
@copy /Y bin\libpeFile.a %PROGRAMS%\local\lib32

