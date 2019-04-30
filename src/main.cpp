#include "stdafx.h"
#include "exe_mod.h"

void dfLink_init()
{
	Linker::addSymbol("@Z", Linker::Type_Absolute, 0, 0);
	Linker::addSymbol("@R", Linker::Type_Relocate, 0, 0);

	Linker::keepSymbol((char*)archStr->hookEntryPoint);
	Linker::keepSymbol((char*)archStr->dllHookStartup);

	Linker::addSymbol(archStr->rawEntryPoint, 
		Linker::Type_Relocate, 0, PeFILE::entryPoint());
	Linker::addSymbol(archStr->dllMainStartup,
		Linker::Type_Relocate, 0, PeFILE::entryPoint());
}

void dfLink_main()
{

}

Linker::Symbol* dfLink_entryPoint()
{
	auto entryPoint = Linker::findSymbol(archStr->hookEntryPoint);
	if(!entryPoint)
		entryPoint = Linker::findSymbol(archStr->dllHookStartup);	
	return entryPoint;
}

int main(int argc, char* argv[])
{
	if(argc < 3) { printf("exe_mod, DeadFish Shitware 2014\n");
		printf(" ussage: exe_mod <src exe/dll> <dest exe/dll>\n"
			"  [link script (.def)] [input objects (.o/.a)]\n\n");
		return 1; 
	} return exe_mod(argc, argv);
}
