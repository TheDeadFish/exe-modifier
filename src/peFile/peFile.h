#ifndef _PEFILE_H_
#define _PEFILE_H_
#include "peHead.h"

namespace PeSecTyp { enum {	Exec = 1, Write = 2, 
	Intd = 4, NoDisc = 8, NoPage = 16, Weird = -1,
	Text = Exec | NoDisc | Intd, Data = Write | NoDisc | Intd,
	RData = NoDisc | Intd,  Bss = Write | NoDisc, IData = Intd  };
}

struct PeReloc : xVectorW<xVectorW<u16>>
{
	void Add(u32 rva); 
	bool Find(u32 rva);
	void Remove(u32 rva);
	void Remove(u32 rva, u32 length);
	void Move(u32 rva, u32 length, int delta);
	
	bool Load(byte* data, u32 size, bool PE64);
	u32 build_size(void);
	void build(byte* data, bool PE64);
};

struct PeExcept
{
	struct RtFunc {
		DWORD start, end, addr; };
	xarray<RtFunc> funcs;
	
	bool Load(byte* data, u32 size, u32 rva);
	void Rebase(u32 rva);
	
	
	
	RtFunc* find(u32 rva, u32 len);
	void kill(u32 rva, u32 len);
};

struct PeSymTab
{
	struct ObjSymbol {
		union { char* name; char Name[8];
		struct { DWORD Name1, Name2; }; };
		DWORD Value; WORD Section;WORD Type;
		BYTE StorageClass; BYTE NumberOfAuxSymbols;
	} __attribute__((packed));
	
	

	struct Symbol {
		char* name; u32 rva;
	};
	
	xArray<Symbol> symbol;
	
	
	void add(char* name, u32 rva);
	
	
	struct StrTable {
		xVector<byte> data;
		u32 add(char* str);
	};

	struct Build_t {
		xArray<ObjSymbol> symData;
		StrTable strTab;
		void xwrite(FILE* fp);
		
		
		
		
		bool hasData() { return symData.len
			|| strTab.data.dataSize; }
	};
};


struct PeFile : PeOptHead
{
	
	auto& dataDir(size_t i) { return dataDir_[i]; }
	auto& dataDir() { return dataDir_; }
	
	struct Section : xarray<byte> {
		DWORD allocSize, baseRva;
		DWORD Characteristics;
		char name[9]; bool noFree;
		
		
		DWORD endRva() { return baseRva+len; }
		DWORD endPage() { return baseRva+allocSize; }
		DataDir dataDir() { return {baseRva,len}; }
		Void rvaPtr(u32 rva) { return data+(rva-baseRva); }
		u32 ptrRva(void* p) { return PTRDIFF(p,data)+baseRva; }
		u32 extent(PeFile& peFile);
		
		
		
		
		
		int resize(PeFile* This, u32 sz);
		
		
		
		
		
		
		
		
		int type(); SHITSTATIC int type(DWORD ch);
		SHITSTATIC bool normSect(cch* name);
		~Section() { if(!noFree) ::free(data); }
		
		
		SHITSTATIC DWORD getType(int type);
		void updateType(int type);
	};
	
	xArray<byte> imageData;
	xarray<byte> boundImp;
	xArray<byte> fileExtra;
	int nSymbols;
	
	cch* load(cch* fileName);
	int save(cch* fileName);
	
	
	
	
	PeFile() { ZINIT; }
	~PeFile(); void Free();
	bool mappMode;
	
	
	// helper functions
	u32 sectAlign(u32); u32 fileAlign(u32);
	SHITSTATIC DWORD calcExtent(Void, DWORD);
	Void patchChk(u64 addr, u32 len);
	Void rvaToPtr(u32 rva, u32 len);
	Void rvaToPtr(u32 len); 
	
	u32 ptrToRva(void* ptr);
	u32 ptrToRva(void* ptr, u32 len);
	
	
	
	u32 chkStr(void* ptr);
	xarray<cch> chkStr2(u32 rva);
	
	xRngPtr<byte> rvaToPtr2(u32 rva, u32 len);
	
	
	
	
	
	
	
	
	// Data directories shorthand
	enum {
		IDE_EXPORT 		= IMAGE_DIRECTORY_ENTRY_EXPORT,
		IDE_IMPORT 		= IMAGE_DIRECTORY_ENTRY_IMPORT,
		IDE_RESOURCE 	= IMAGE_DIRECTORY_ENTRY_RESOURCE,
		IDE_EXCEPTION 	= IMAGE_DIRECTORY_ENTRY_EXCEPTION,
		IDE_SECURITY 	= IMAGE_DIRECTORY_ENTRY_SECURITY,
		IDE_BASERELOC 	= IMAGE_DIRECTORY_ENTRY_BASERELOC,
		IDE_DEBUG 		= IMAGE_DIRECTORY_ENTRY_DEBUG,
		IDE_ARCH 		= IMAGE_DIRECTORY_ENTRY_ARCHITECTURE,
		IDE_GLOBL	 	= IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
		IDE_TLS 		= IMAGE_DIRECTORY_ENTRY_TLS,
		IDE_CONFIG 		= IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
		IDE_BOUNDIMP	= IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
		IDE_IATABLE		= IMAGE_DIRECTORY_ENTRY_IAT,
		IDE_DELAYIMP	= IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
		IDE_COM_DESC	= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR	
	};
	
	// section interface
	xArray<Section> sects; Section 
	*extendSect, *rsrcSect, *relocSect, *pdataSect;
	Section* rvaToSect(u32 rva, u32 len);
	Section* ptrToSect(void* ptr, u32 len);
	
	int sectCreate(cch* name, DWORD ch);
	int sectCreate2(cch* name, int type);
	void sectResize(Section* sect, u32 size);
	
	int iSect(Section* sect) { return sect-sects.data; }
	int iSect2(Section* sect) { 
		return !sect ? -1 : iSect(sect); }
	
	//void rebase(u32 delta);
	
	void setRes(void* data, DWORD size);
	
	static __fastcall xarray<byte> dataDirSectChk(
		Section* sect, DataDir* dir, cch* name);
		
	
	
	
	PeReloc relocs;
	PeExcept pdata;
	PeSymTab symtab;
	
	
	
private:
	void getSections_();
	void symTab_build(PeSymTab::Build_t& sd);
};

#endif
