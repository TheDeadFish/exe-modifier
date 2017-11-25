call exmod.bat x64
gcc ..\helloWorld.cc %CFLAGS2% -c
exe_mod calc.exe hello_calc.exe calc.def helloWorld.o 

