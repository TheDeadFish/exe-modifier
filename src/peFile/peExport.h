#ifndef _PEEXPORT_H_
#define _PEEXPORT_H_
#include "peBlock.h"

struct PeExport
{
	DWORD funcTabRva; xstr dllName; 
	DWORD Characteristics; DWORD TimeDateStamp;
	WORD MajorVersion;  WORD MinorVersion;
	DWORD OrdinalBase; bool mustRebuild;
		
	struct ExportSlot { xstr name, frwd;
		uint ord; uint rva; static int sortFn(
		const ExportSlot& a, const ExportSlot& b); };
	xArray<ExportSlot> exports; FreeLst freeLst;
	PeFile& peFile() { return *freeLst.peFile; }
	u32 ptrSize() { return peFile().PE64 ? 8 : 4; }
	
	PeExport() { ZINIT; }
	
	
	
	int load(PeFile& peFile);
	xarray<PeBlock> getBlocks(void);
	void build(PeBlock* blocks);
	
	
	
	void setRva(ExportSlot& slot, DWORD rva);
	void setFrwd(ExportSlot& slot, cch* frwd);
	void setRva(cch* name, DWORD rva);
	
	
	
	
	//int add(char* name, int ord);
	
	
	ExportSlot* find(cch* name);
	ExportSlot* find(uint ord);
	
	
	//
	
	
	DEF_RETPAIR(find_t, ExportSlot*, slot, int, err);
	find_t find(cch* name, uint ord);
	ExportSlot& add(cch* name, uint ord);
	
	
	
	//SHITSTATIC char* makeName(cch* name, int ord);
};

#endif
