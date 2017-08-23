#include "stdafx.h"
#include "peFile_.h"

namespace PeFILE {

	PeFile peFile;
	PeImport peImp;
	FreeLst freeList;
	
	
	
#include "peExport.cpp"
		
int Import_Find(char* dllName, char* importName){
	if(Exports_HasFunc(dllName, importName)) {
		fatal_error("Import_Find: cannot import from self, %s\n", 
			importName); }
	return peImp.find(dllName, importName);
}

int Import_Add(char* dllName, char* importName){
	if(int exportRva = Exports_HasFunc(dllName, importName))
		return exportRva;
	peImp.add(dllName, importName); return 0;
}


const char* load(const char* fileName)
{
	cch* result = peFile.load(fileName);
	if(result) return result; freeList.peFile = &peFile;
	if(int ec = peImp.load(peFile)) { static cch* const impErr[] = { NULL, "Error_Imports1", 
		"Error_Imports2", "Error_Imports3", "Error_Imports4", "Error_Imports5" };
		return impErr[ec]; }
	if(result = Exports_Load()) return result;
	return NULL;
}

const char* save(const char* fileName)
{
	if(peFile.save(fileName))
		return "Error_FailedToOpen";
	return NULL;
}


void clearSpace(int rva, int length, int offset)
{
	Void ptr = peFile.rvaToPtr(rva, length);
	assert(ptr != NULL); memset(ptr, 0, length);
	if((length -= offset) > 0) freeList.
		mark(rva+offset, length, 1);
}

void allocBlocks(PeBlock* blocks, int nBlocks) {
	::allocBlocks({blocks, nBlocks}, peFile, &peImp, &freeList); }

};
