#include "exe_mod.h"

void dfLink_init()
{
	assert(PeFile::entryPoint() == 0);
	Linker::addSymbol("___main", Linker::Type_Absolute, 0, 0);
}

void dfLink_main()
{
	// detect main type
	int mainType, mainSymb;
	static const char* entryName[] = {"_main", "_wmain",
		"_WinMain@16", "_WinMainW@16"};
	for(int i = 0; i < 4; i++) {
		int mainSymb = Linker::findSymbol(entryName[i]);
		if((mainSymb >= 0)
		&&(Linker::symbols[mainSymb].section >= 0)) {
			mainType = i; goto FOUND_MAIN; }
	} fatal_error("entry point not found");
	
	// 
	
	
	
	
FOUND_MAIN:;
	xvector entryData = {0};
	
	

	
	
	
	
	
	
	
	
	






}

int dfLink_entryPoint()
{
	int entryPoint = Linker::findSymbol("_RawEntryPoint");
	if(entryPoint < 0)
		entryPoint = Linker::findSymbol("_DllMainCRTStartup@12");
	if(entryPoint < 0)
		fatal_error("entry point not found");
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
