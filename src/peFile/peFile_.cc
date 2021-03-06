#include "stdafx.h"
#include "peFile_.h"

namespace PeFILE {

	PeFile peFile;
	PeImport peImp;
	PeExport peExp;
	FreeLst freeList;

		
int Import_Find(char* dllName, char* importName){
	//if(Exports_HasFunc(dllName, importName)) {
	//	fatal_error("Import_Find: cannot import from self, %s\n", 
	//		importName); }
	return peImp.find(dllName, importName);
}

int Import_Add(char* dllName, char* importName){
	//if(int exportRva = Exports_HasFunc(dllName, importName))
	//	return exportRva;
	peImp.add(dllName, importName); return 0;
}


const char* load(const char* fileName)
{
	freeList.peFile = &peFile;
	IFRET(peFile.load(fileName));
	if(int ec = peImp.load(peFile)) {
		return xstrfmt("Error_Imports:%d", ec); }
	if(int ec = peExp.load(peFile)) {
		return xstrfmt("Error_Exports:%d", ec); }
	return NULL;
}

const char* save(const char* fileName)
{
	if(peFile.save(fileName))
		return "Error_FailedToOpen";
	return NULL;
}

void close()
{
	pRst(&peImp);
	pRst(&peExp);
	peFile.Free();
}

void clearSpace(int rva, int length, int offset)
{
	Void ptr = peFile.rvaToPtr(rva, length);
	assert(ptr != NULL); memset(ptr, 0, length);
	if((length -= offset) > 0) freeList.
		mark(rva+offset, length, 1);
}

void allocBlocks(PeBlock* blocks, int nBlocks) {
	::allocBlocks({blocks, nBlocks}, peFile, &peImp, &peExp, &freeList); }

};
