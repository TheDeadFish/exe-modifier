setlocal
call egcc.bat
gcc %CCFLAGS2% stubgen.cc -lstdshit -o stubgen.exe
stubgen.exe
del *.exe obj32\*.o obj64\*.o
