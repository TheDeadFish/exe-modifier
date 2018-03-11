#ifndef _PEBLOCK_H_
#define _PEBLOCK_H_
#include "peFile.h"

struct FreeLst_t { u32 rva, end; };
struct FreeLst2_t : FreeLst_t { word align, iSect;
	bool merge(FreeLst2_t& that, int sz); 
	static __thiscall void mark(void* xa, u32 sz,
		u32 rva, u32 end, u32 align, u32 iSect); 
};

struct FreeLst0 : xArray<FreeLst_t> 
{
	void mark(u32 rva, u32 end);
};

struct FreeLst : xArray<FreeLst2_t> 
{
	PeFile* peFile; 
	void init(PeFile* pe) { peFile = pe; }
	Void mark(u32 rva, u32 len, u32 align);
	Void mark(u32 rva, u32 len, u32 align, bool doMark);
	char* markStrDup(u32 rva);
};

struct PeBlock {
	WORD type; WORD align;
	DWORD baseRva; DWORD length; 
	int lnSect; union {
	void* data; int peSect; };
	int order() const; static int cmpFn(
		const PeBlock& a, const PeBlock& b);
};

struct PeImport; struct PeExport;
void allocBlocks(xarray<PeBlock> blocks, PeFile& 
	peFile, PeImport* peImp, PeExport* peExp, FreeLst* freeLst);


#endif
