#include "stdafx.h"
#include "exe_mod.h"

void dfLink_init()
{
	Linker::addSymbol("_RawEntryPoint",	Linker::Type_Relocate,
		-1, PeFile::rvaToAddr(PeFile::entryPoint()));
	Linker::addSymbol("_DllMainCRTStartup@12", Linker::Type_Relocate,
		-1, PeFile::rvaToAddr(PeFile::entryPoint()));
}

void dfLink_main()
{

}

int dfLink_entryPoint()
{
	int entryPoint = Linker::findSymbol("_HookEntryPoint");
	if(entryPoint < 0)
		entryPoint = Linker::findSymbol("_DllHookCRTStartup@12");	
	return entryPoint;
}

int main(int argc, char* argv[])
{
	GetModuleFileName(NULL, exeName, MAX_PATH);
	exePathLen = getPathLen(exeName);

	if(argc < 4) { printf("exe_mod, DeadFish Shitware 2014\n");
		printf(" ussage: exe_mod <src exe/dll> <dest exe/dll>\n"
			"  [link script (.def)] [input objects (.o/.a)]\n\n");
		return 1; 
	} return exe_mod(argc, argv);
}