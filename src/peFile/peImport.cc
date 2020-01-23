#include "stdafx.h"
#include "peImport.h"

PeImport::ImportSlot::ImportSlot(cch* name, USHORT hint)
{
	if(!name) {	this->name.init(xstrfmt("#%d", hint));
	} else { if(name[0] == '#') hint = atoi(name+1);
		this->name.init(xstrdup(name)); } this->hint = hint;
}

int PeImport::load(PeFile& peFile)
{
	freeLst.peFile = &peFile;
	u32 impRva = peFile.dataDir(peFile.IDE_IMPORT).rva;
	if(!impRva) return 0; 
	while(1) {
		// mark directory entry
		IMAGE_IMPORT_DESCRIPTOR* impDesc = peFile.rvaToPtr(
			impRva, sizeof(IMAGE_IMPORT_DESCRIPTOR));
		if(!impDesc) return 1; freeLst.mark(impRva, sizeof(
		IMAGE_IMPORT_DESCRIPTOR), 4); if(!impDesc->Name) return 0;
		impRva += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		
		// read directory entry
		auto dllName = peFile.chkStr2(impDesc->Name);
		if(!dllName) return 2;
		freeLst.mark(impDesc->Name, dllName.len, 2);
		ImportDir& impDir = imports.push_back(dllName.data, 
			impDesc->TimeDateStamp, impDesc->FirstThunk);

		// get name table
		bool hasOft = impDesc->OriginalFirstThunk;
		DWORD nameRva = !hasOft ? impDesc->FirstThunk :
			impDesc->OriginalFirstThunk;
		int ptrSize = this->ptrSize();
		
		// read name table
		while(1) {
			DWORD* Name = peFile.rvaToPtr(nameRva, ptrSize);
			if(!Name) return 4; if(hasOft) freeLst.mark(nameRva, 
				ptrSize, ptrSize);	nameRva += ptrSize;
			if(RC(Name,ptrSize-1) < 0) { impDir.push_back((cch*)0, *Name);
			} else { if(*Name == 0) break;
				IMAGE_IMPORT_BY_NAME* name = peFile.rvaToPtr(*Name, 3);
				u32 strSz = peFile.chkStr2(*Name+2).len; if(!strSz) {
				return 4; } freeLst.mark(*Name, strSz+2, 2);
				impDir.push_back((char*)name->Name, name->Hint);
			}
		}
	}
}

bool PeImport::mustRebuild(void)
{
	for(auto& dir : imports)
		if(!dir.FirstThunk) return true;
	return false;
}

xarray<PeBlock> PeImport::getBlocks(void)
{
	// allocate block list
	int nBlocks = imports.len*3+1;
	for(auto& dir : imports) nBlocks += dir.len;
	PeBlock* blocks = xCalloc(nBlocks);
	for(int i = 0; i < nBlocks; i++) { blocks[i].type 
		= PeSecTyp::IData; blocks[i].align = 1; }
	PeBlock* curBlock = blocks;
		
	// import address table
	u32 ptrSize = this->ptrSize();
	for(auto& dir : imports) if(!dir.FirstThunk) {
		curBlock->length = (dir.len+1) * ptrSize; 
		curBlock->type = PeSecTyp::Data; curBlock++; }
		
	// import directory table
	curBlock->length = (imports.len+1) *
		sizeof(IMAGE_IMPORT_DESCRIPTOR);
	curBlock->align = 4; curBlock++;
	
	// original first thunks
	for(auto& dir : imports) { 
		curBlock->length = (dir.len+1) * ptrSize;
		curBlock->align = ptrSize; curBlock++; }
	
	// import names
	for(auto& dir : imports) { curBlock->length = 
		strlen(dir.DllName)+1; curBlock++; }
	for(auto& dir : imports) for(auto& slot : dir) {
		if(slot.HasName()) { curBlock->length
			= strlen(slot.name)+3; curBlock++;} }
	
	return {blocks, curBlock};
}

void PeImport::build(PeBlock* blocks)
{
	// import address table
	if(blocks == NULL) return; peFile().boundImp.Clear();
	for(auto& dir : imports) if(dir.FirstThunk == 0) {
		dir.FirstThunk = blocks->baseRva; blocks++; }

	// import descriptor
	peFile().dataDir(peFile().IDE_IMPORT).rva = blocks->baseRva;
	peFile().dataDir(peFile().IDE_IMPORT).size = blocks->length;
	IMAGE_IMPORT_DESCRIPTOR* impDesc = Void(blocks->data);
	memset(impDesc, 0, blocks->length); blocks++;
	for(int i = 0; i < imports.len; i++, blocks++) {
		impDesc[i].FirstThunk = imports[i].FirstThunk;
		impDesc[i].OriginalFirstThunk = blocks->baseRva; }
	for(int i = 0; i < imports.len; i++, blocks++) {
		strcpy((char*)blocks->data, imports[i].DllName);
		impDesc[i].Name = blocks->baseRva; }
		
	// import names
	for(int i = 0; i < imports.len; i++) {
		u32* thnkPos1 = peFile().rvaToPtr(impDesc[i].OriginalFirstThunk);
		u32* thnkPos2 = peFile().rvaToPtr(impDesc[i].FirstThunk);
	
	
	for(auto* slot = imports[i].data, * slotEnd = 
		imports[i].end(); slot <= slotEnd; slot++)
	{
		// write name
		u32 lowVal=0, highVal; if(slot == slotEnd) goto END;
		if(slot->HasName()) { RW(blocks->data) = slot->hint;
			strcpy((char*)blocks->data+2, slot->name);
			lowVal = blocks->baseRva; blocks++; END: highVal = 0; } 
		else { highVal = 0x80000000; lowVal = slot->hint; }
		
		// update thunks
		if(!peFile().PE64()) { lowVal |= highVal; } 
		WRI(thnkPos1, lowVal); WRI(thnkPos2, lowVal);
		if(peFile().PE64()) { WRI(thnkPos1, highVal);
			WRI(thnkPos2, highVal); }
	}}
}

int PeImport::find(cch* dllName, cch* importName)
{
	for(auto& impDir : imports)
	if(!stricmp(impDir.DllName, dllName)) {
		for(int i = 0; i < impDir.len; i++)
		if(!strcmp(impDir[i].name, importName)) {
			int ret = impDir.FirstThunk; if(!NULL_CHECK(ret))
			ret += i*ptrSize();	return ret; }
		if(!impDir.FirstThunk) return INT_MIN | 
			PTRDIFF(&impDir, imports.data);
	} return -1;
}

void PeImport::add(cch* dllName, cch* importName)
{
	// locate existing import
	int result = find(dllName, importName);
	if(result > -1) return;

	// allocate import directory
	ImportDir* impDir;
	if(result == -1) {
		impDir = &imports.push_back(dllName, 0, 0);
	} else { impDir = Void(imports.data,
		uint(result)-0x80000000);
	} impDir->push_back(importName, 0);
}
