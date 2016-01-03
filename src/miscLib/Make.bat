setlocal
call egcc.bat
gcc *.s -c
ar -rcs ..\..\bin\libmisc.a *.o
move /Y main.o ..\..\bin\main.o
move /Y wmain.o ..\..\bin\wmain.o
del *.o
