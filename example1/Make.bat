call exmod.bat
gcc helloWorld.cc %CFLAGS2% -c
exe_mod calc.exe hello_calc.exe calc.def helloWorld.o 
del *.o
