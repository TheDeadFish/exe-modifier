@setlocal
@call egcc.bat
@echo off
gcc stdafx.cc -I. %CFLAGS2% -g -c -o ..\stdafx.o
gcc -x c++-header stdafx.h %CFLAGS2% -g -o ..\stdafx.h.gch
