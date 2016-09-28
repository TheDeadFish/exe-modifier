@setlocal
@call egcc.bat
@echo off
gcc stdafx.cc -I. %CCFLAGS2% -g -c -o ..\stdafx.o
gcc -x c++-header stdafx.h %CCFLAGS2% -g -o ..\stdafx.h.gch
