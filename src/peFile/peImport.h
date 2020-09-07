#ifndef _PEIMPORT_H_
#define _PEIMPORT_H_
#include "peBlock.h"

struct PeImport
{
	struct ImportSlot {
		xstr name; USHORT hint;
		bool HasName() { return (name[0] != '#'); }
		ImportSlot(cch* name, USHORT hint); 
	};
	struct ImportDir : xArray<ImportSlot> { 
		xstr DllName; DWORD TimeDateStamp, FirstThunk;
		ImportDir(cch* str, DWORD ts, DWORD ft) : DllName(
		xstrdup(str)), TimeDateStamp(ts), FirstThunk(ft) {}
	};
	
	xArray<ImportDir> imports; FreeLst freeLst;
	PeFile& peFile() { return *freeLst.peFile; }
	u32 ptrSize() { return peFile().ptrSize(); }
	
	int load(PeFile& peFile);
	bool mustRebuild(void);
	xarray<PeBlock> getBlocks(void);
	void build(PeBlock* blocks);
	int find(cch* dllName, cch* importName);
	void add(cch* dllName, cch* importName);
	
	struct Detatch_t { u32 dir, iat, iatEnd; };
	Detatch_t detatch();
};	

#endif
