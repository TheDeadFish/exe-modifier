sub errorExit(errStr)
	Wscript.Echo errStr
	Wscript.Quit 1
end sub
sub checkExist(filePath)
	If((fso.FileExists(filePath) = False)and(fso.FolderExists(filePath) = False)) Then
		errorExit("path does not exist: " + filePath)
	End If
end sub

rem locate library directory
mingwBase = InputBox("Please enter path to MinGW", _
    "exe modifier setup", "C:\MinGW")
if(mingwBase = "") Then
	Wscript.Quit 1
End If
Set fso = CreateObject("Scripting.FileSystemObject")
checkExist(mingwBase)
checkExist(mingwBase+"\lib\gcc\mingw32")
Set objFolder = fso.GetFolder(mingwBase+"\lib\gcc\mingw32")
lib_path = ""
For Each Subfolder in objFolder.SubFolders
	IF(fso.FileExists(Subfolder.Path+"\libgcc.a")) Then
		lib_path = Subfolder.Path
		Exit For
	End If
Next
If lib_path = "" Then
	errorExit("libgcc.a could not be found")
End If

rem create config file
Set MyFile = fso.CreateTextFile("exe_mod.cfg", True)
MyFile.WriteLine("lib_path=" + lib_path + ";" + mingwBase + "\local\lib;" + mingwBase + "\lib")
MyFile.WriteLine("def_libs=-lmisc -lmingw32 -lmingwex -lgcc -lmoldname -lmisc -lmsvcrt -lgdi32 -lcomdlg32 -ladvapi32 -lshell32 -luser32 -lkernel32")
MyFile.WriteLine("keep_list=_HookEntryPoint _DllHookCRTStartup@12 .idata$7")
MyFile.Close()

rem create exmod.bat
batchBase = InputBox("Please enter path to create batch files (egcc.bat and exmod.bat)", "exe modifier setup", "C:\WINDOWS\system32")
If(batchBase = "") Then
	Wscript.Quit 1
End If
Set MyFile = fso.CreateTextFile(batchBase + "\exmod.bat", True)
MyFile.WriteLine("@if ""%EEXE_MOD%"" == ""YES"" GOTO END")
MyFile.WriteLine("@call egcc.bat")
MyFile.WriteLine("@path=%path%;" + fso.GetAbsolutePathName("."))
MyFile.WriteLine("@set EEXE_MOD=YES")
MyFile.WriteLine(":END")

rem create egcc.bat
Set MyFile = fso.CreateTextFile(batchBase + "\egcc.bat", True)
MyFile.WriteLine("@if ""%EGCC_BAT%"" == ""YES"" GOTO END")
MyFile.WriteLine("@path=%path%;" + mingwBase + "\bin")
MyFile.WriteLine("@set GCCDIR=" + mingwBase)
MyFile.WriteLine("@set prefix=" + mingwBase + "\local")
MyFile.WriteLine("@set C_INCLUDE_PATH=" + mingwBase + "\local\include")
MyFile.WriteLine("@set CPLUS_INCLUDE_PATH=" + mingwBase + "\local\include")
MyFile.WriteLine("@set LIBRARY_PATH=" + mingwBase + "\local\lib")
MyFile.WriteLine("@set CFLAGS= -D_WIN32_WINNT=0x0501 -march=i486 -mtune=generic -Os -fomit-frame-pointer -mpreferred-stack-boundary=2 -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -std=gnu++11 -Wno-narrowing -Wno-pmf-conversions -ffunction-sections")
MyFile.WriteLine("@set CFLAGS2=%CFLAGS% -mno-accumulate-outgoing-args  -mno-stack-arg-probe")
MyFile.WriteLine("@set LFLAGS=-s -Wl,-gc-sections")
MyFile.WriteLine("@set CXXFLAGS=CFLAGS")
MyFile.WriteLine("@set EGCC_BAT=YES")
MyFile.WriteLine(":END")
MyFile.Close()
