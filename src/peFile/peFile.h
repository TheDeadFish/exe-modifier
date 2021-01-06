#ifndef _PEFILE_H_
#define _PEFILE_H_
#include "peHead.h"
#include "peReloc.h"
#include "peSymTab.h"
#include "peExcept.h"

namespace PeSecTyp { enum {	Exec = 1, Write = 2, 
	Intd = 4, NoDisc = 8, NoPage = 16, Weird = -1,
	Text = Exec | NoDisc | Intd, Data = Write | NoDisc | Intd,
	RData = NoDisc | Intd,  Bss = Write | NoDisc, IData = Intd  };
}


struct PeFile
{
	struct DataDir {DWORD rva, size; };

	IMAGE_NT_HEADERS64* inh;

	IMAGE_OPTIONAL_HEADER64& ioh() { return inh->OptionalHeader; }
	
	u64 ImageBase;
	
	bool PE64() { return peHead64(inh); }
	u32 ptrSize() { return PE64() ? 8 : 4; }
	
	DataDir dataDir(size_t i);
	bool setDataDir(size_t i, DataDir dd);
	
	struct Section {
		byte* data;
		DWORD allocSize;
		
		IMAGE_SECTION_HEADER* ish;
		
		DWORD& baseRva() { return ish->VirtualAddress; }
		DWORD& len() { return ish->Misc.VirtualSize; }
		

		bool noFree;
		
		
		byte* end() { return data + len(); }
		
		
		
		
		DWORD endRva() { return baseRva()+len(); }
		DWORD endPage() { return baseRva()+allocSize; }
		DataDir dataDir() { return {baseRva(),len()}; }
		Void rvaPtr(u32 rva) { return data+(rva-baseRva()); }
		u32 ptrRva(void* p) { return PTRDIFF(p,data)+baseRva(); }
		u32 extent(PeFile& peFile);
		
		
		int namecmp(cch* name);
		
		
		
		
		
		
		int resize(PeFile* This, u32 sz);
		
		
		
		
		
		
		
		
		int type(); SHITSTATIC int type(DWORD ch);
		SHITSTATIC bool normSect(cch* name);
		~Section() { if(!noFree) ::free(data); }
		
		
		SHITSTATIC DWORD getType(int type);
		void updateType(int type);
	};
	
	xArray<byte> imageData;
	
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
	
	
	u32 addrToRva(u64 addr);
	Void addrToPtr(u64 addr);
	Void addrToPtr(u64 addr, u32 len);
	
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
		Section* sect, DataDir dir, cch* name);
		
	void boundImp_clear() {
		setDataDir(IDE_BOUNDIMP, {}); 
	}
		
	
	
	
	PeReloc relocs;
	PeExcept pdata;
	PeSymTab symtab;

private:
	void getSections_();
	void symTab_build(PeSymTab::Build_t& sd);
};

#endif
