setlocal
call egcc.bat
gcc -m32 %CCFLAGS2% stubgen.cc -lstdshit -o stubgen.exe
endlocal
setlocal
call egcc64.bat
stubgen.exe
del *.exe obj32\*.o obj64\*.o
