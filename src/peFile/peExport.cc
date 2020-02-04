#include "stdafx.h"
#include "peExport.h"
#include <time.h>

int PeExport::load(PeFile& peFile)
{
	freeLst.peFile = &peFile;
	uint expRva = peFile.dataDir(peFile.IDE_EXPORT).rva;
	uint expSize = peFile.dataDir(peFile.IDE_EXPORT).size;
	if(!expRva) return 0; 

	// read export directory
	PIMAGE_EXPORT_DIRECTORY expBase = freeLst.mark(expRva, expSize, 4);
	{ char* str; if(!expBase || (expSize < sizeof(*expBase))
	|| !(str = freeLst.markStrDup(expBase->Name))) return 1; 
	dllName.init(xstrdup(str)); } 
	OrdinalBase = expBase->Base;
	RI(&MajorVersion) = RI(&expBase->MajorVersion);
	TimeDateStamp = expBase->TimeDateStamp;
	Characteristics = expBase->Characteristics;
	
	// read functions table
	uint expEnd = expRva+expSize;
	funcTabRva = expBase->AddressOfFunctions;
	for(DWORD i = 0; i < expBase->NumberOfFunctions; i++) { DWORD* Func = 
		freeLst.mark(expBase->AddressOfFunctions+i*4, 4, 4); if(!Func) return 2; 
		auto& slot = exports.xnxalloc(); slot.ord = OrdinalBase+i; slot.name.init(0);
		bool fw = inRng1(*Func, expRva, expEnd); slot.rva = !fw ? *Func : 0;
		slot.frwd.init(fw ? freeLst.markStrDup(*Func) : 0);
	}

	// read names table
	for(uint i = 0; i < expBase->NumberOfNames; i++) { 
		DWORD ord, *pName; cch* name; WORD* pOrd = 
		freeLst.mark(expBase->AddressOfNameOrdinals+i*2, 2, 2);
		if(!pOrd || ((ord = *pOrd) >= exports.len)) return 3;
		pName = freeLst.mark(expBase->AddressOfNames+i*4, 4, 4);
		if(!pName || !(name = freeLst.markStrDup(*pName))) return 4;
		exports[ord].name.init(xstrdup(name)); 
		exports[ord].nord = i+1;
	}
	
	return 0;
};

SHITSTATIC uint strsize(cch* str) {
	if(!str) return 0;
	return strlen(str)+1; }

static inline cch* nzStr(cch* str) {
	return str ? str : ""; }

xarray<PeBlock> PeExport::getBlocks(void)
{
	// determin range
	uint nNames = 0;
	uint ordMin = -1, ordMax = -1;
	for(auto& slot : exports) {
		if(slot.name) nNames++;
		if(slot.ord) { min_ref(ordMin, slot.ord);
			max_ref(RI(&ordMax), int(slot.ord)); }
	}
	
	// apply ranges
	if(!OrdinalBase) OrdinalBase = 1;
	if((!isNeg(ordMin))&&((OrdinalBase >= ordMin)
	||((OrdinalBase+exports.len) < (ordMax+1))))
		OrdinalBase = ordMin;
	uint nFuncs = max(int(exports.len), 
		int(ordMax+1 - OrdinalBase));
	
	// assign ordinals
	{qsort(exports.data, exports.len, [](const ExportSlot& a, 
		const ExportSlot& b){ return int(a.ord-b.ord); });
	auto* srcPos = exports.begin(); auto* dstPos = srcPos;
	auto* endPos = exports.end(); uint curOrd = OrdinalBase; 
	for(;(srcPos < endPos)&&(!srcPos->ord); srcPos++);
	for(;dstPos < srcPos; curOrd++, dstPos++) { 
		if((srcPos == endPos)||(srcPos->ord > curOrd)) { 
			dstPos->ord = curOrd; } else { srcPos = 
		Void(memswap(srcPos, dstPos, sizeof(*srcPos))); }
	}}
		
	// allocate block list
	PeBlock* blocks = xCalloc(nNames+2);
	for(uint i = 0; i < nNames+2; i++) { blocks[i].type 
		= PeSecTyp::IData; blocks[i].align = 1; }
	blocks[0].lnSect = nFuncs; blocks[1].lnSect = nNames;
		
	// export directory + tables
	uint len = sizeof(IMAGE_EXPORT_DIRECTORY) +
		(nFuncs * 4) + (nNames * 6);
	for(auto& slot : exports) if(slot.frwd) { 
		len += (slot.rva = strsize(slot.frwd)); }
	blocks[0].length = len; blocks[0].align = 4;
	
	// export names
	qsort(exports.data, exports.len, [](const 
		ExportSlot& a, const ExportSlot& b) { return 
		strcmp(nzStr(a.name.data), nzStr(b.name.data)); });
	blocks[1].length = strsize(dllName);
	PeBlock* curBlock = blocks+2;
	for(auto& slot : exports) if(slot.name) { curBlock
		->length = strsize(slot.name); curBlock++; };
		
	return {blocks, curBlock};
}



void PeExport::build(PeBlock* blocks)
{
	// write export directory
	if(blocks == NULL) return;
	peFile().dataDir(PeFile::IDE_EXPORT) = 
		{blocks[0].baseRva, blocks[0].length};
	PIMAGE_EXPORT_DIRECTORY expBase = Void(blocks[0].data);
	expBase->Base = OrdinalBase;
	RI(&expBase->MajorVersion) = RI(&MajorVersion);
	expBase->TimeDateStamp = TimeDateStamp;
	expBase->Characteristics = Characteristics;
	expBase->Name = blocks[1].baseRva;
	strcpy((char*)blocks[1].data, dllName);
	
	// write table addresses
	expBase->NumberOfFunctions = blocks[0].lnSect;
	expBase->NumberOfNames = blocks[1].lnSect;
	expBase->AddressOfFunctions = blocks[0].baseRva	
		+ sizeof(IMAGE_EXPORT_DIRECTORY);
	expBase->AddressOfNames = expBase->AddressOfFunctions 
		+ expBase->NumberOfFunctions * 4;	
	expBase->AddressOfNameOrdinals = expBase->AddressOfNames
		+ expBase->NumberOfNames * 4;
	funcTabRva = expBase->AddressOfFunctions;
		
	// write names
	DWORD* funcTab = Void(expBase+1);
	DWORD* namePos = funcTab+blocks[0].lnSect;
	WORD* ordPos = PW(namePos+blocks[1].lnSect);
	for(auto& slot : exports) if(slot.name) {
		WRI(ordPos, slot.ord-OrdinalBase);
		WRI(namePos, blocks[2].baseRva);
		char* data = PC(blocks[2].data); 
		blocks++; strcpy(data, slot.name);		
	}
	
	// write the functions
	char* frwdPos = PC(ordPos);
	for(auto& slot : exports) {
		uint funcIdx = slot.ord-OrdinalBase;
		if(!slot.frwd) { 
			funcTab[funcIdx] = slot.rva;
		} else { 
			funcTab[funcIdx] = peFile
				().ptrToRva(frwdPos);
			strcpy(frwdPos, slot.frwd);
			frwdPos += slot.rva;
		}
	}
}

PeExport::ExportSlot* PeExport::find(uint ord)
{
	if(ord)for(auto& slot : exports) { 
	if(slot.ord == ord) return &slot; } return NULL;
}

PeExport::ExportSlot* PeExport::find(cch* name)
{
	if(name){ if(*name == '#') return find(atoi(name+1));
	for(auto& slot : exports) { if((slot.name)&&(!strcmp(slot.
	name, name))) return &slot; }} return NULL;
}

PeExport::find_t PeExport::find(cch* name, uint ord)
{
	auto* nSlot = find(name);
	if(ord) { auto* oSlot = find(ord);
	if(name) { if(nSlot != oSlot) return {0, 1};
		return {oSlot, oSlot ? -1 : 0}; }
		return {oSlot, (oSlot && oSlot->name)
		? -2 : 0}; } return {nSlot, 0};
}

PeExport::ExportSlot& PeExport::add(cch* name, uint ord)
{
	mustRebuild = true;
	auto& slot = exports.xnxalloc();
	memset(&slot, 0, sizeof(slot));
	slot.name = xstrdup(name);
	slot.ord = ord; return slot;
}

void PeExport::setRva(ExportSlot& slot, DWORD rva)
{
	mustRebuild = true;
	if(slot.frwd || slot.rva) {
		TimeDateStamp = time(0); }
	free_ref(slot.frwd.data);
	slot.rva = rva;
}

void PeExport::setFrwd(ExportSlot& slot, cch* frwd)
{
	mustRebuild = true;
	if(slot.frwd || slot.rva) {
		TimeDateStamp = time(0); } 
	slot.rva = 0; 
	xstrdupr(slot.frwd.data, frwd);
}


void PeExport::setRva(cch* name, DWORD rva)
{
	auto* slot = find(name);
	assert(slot && (slot->ord > 0));
	DWORD* ptr = peFile().rvaToPtr(funcTabRva 
		+ (slot->ord - OrdinalBase)*4, 4);
	assert(ptr != NULL); if(*ptr)
	TimeDateStamp = time(0); *ptr = rva;
}
