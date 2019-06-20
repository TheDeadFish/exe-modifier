#ifndef _PEFILE_H_
#define _PEFILE_H_

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

struct PeFile
{
	// IMAGE_FILE_HEADER
	struct { WORD Machine, junk1;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD junk2, Characteristics; };
	
	// IMAGE_OPTIONAL_HEADER
	struct { bool PE64, junk3;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD AddressOfEntryPoint;
	ULONGLONG ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	ULONGLONG SizeOfStackReserve;
	ULONGLONG SizeOfStackCommit;
	ULONGLONG SizeOfHeapReserve;
	ULONGLONG SizeOfHeapCommit;
	DWORD LoaderFlags; };
	
	struct DataDir {DWORD rva, size; };
	DataDir dataDir[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	
	
	struct Section : xArray<byte> {
		DWORD allocSize, baseRva;
		DWORD Characteristics;
		char name[9]; 
		
		
		DWORD endRva() { return baseRva+size; }
		DWORD endPage() { return baseRva+allocSize; }
		DataDir dataDir() { return {baseRva,size}; }
		Void rvaPtr(u32 rva) { return data+(rva-baseRva); }
		u32 ptrRva(void* p) { return PTRDIFF(p,data)+baseRva; }
		u32 extent(PeFile& peFile);
		
		
		
		
		
		int resize(PeFile* This, u32 sz);
		
		
		
		
		
		int type(); SHITSTATIC int type(DWORD ch);
		SHITSTATIC bool normSect(cch* name);
	};
	
	xArray<byte> dosHeadr;
	xArray<byte> boundImp;
	xArray<byte> fileExtra;
	
	cch* load(cch* fileName);
	int save(cch* fileName);
	
	
	
	
	PeFile() { ZINIT; }
	~PeFile(); void Free();
	
	
	
	
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
	*extendSect, *rsrcSect, *relocSect;
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
	
private:
	void getSections_();
};

#endif
