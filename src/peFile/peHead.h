#pragma once

enum { IMAGE_SCN_CNT_DATA = 
	IMAGE_SCN_CNT_INITIALIZED_DATA | 
	IMAGE_SCN_CNT_UNINITIALIZED_DATA
};

struct PeDataDir {DWORD rva, size; };
struct PeDataDirX { DWORD LoaderFlags;
	DWORD dataDirSize; PeDataDir dataDir[16]; };

struct PeOptHead
{
	WORD Machine;
	WORD Characteristics;
	DWORD TimeDateStamp;
	
	WORD Magic;
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
	DWORD LoaderFlags;
	
	struct DataDir {DWORD rva, size; };
	DataDir dataDir_[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	
	
	//u32 ioh_calcSize(void);
	
	
	
	//void ioh_save(
	
	bool PE64() { return u8(Magic>>8) == 2; }
	u32 ptrSize() { return PE64() ? 8 : 4; }
	
	
	//xarray<byte> build(xarray<byte> dosHead
	
	IMAGE_SECTION_HEADER* ioh_pack(IMAGE_NT_HEADERS64* inh);
	IMAGE_SECTION_HEADER* ioh_unpack(IMAGE_NT_HEADERS64* inh);
	
	
	
	
	
	
	
};


SHITCALL int peMzChk(void* data, u32 size);
SHITCALL int peHeadChk(IMAGE_NT_HEADERS64* inh, u32 e_lfanew, u32 size);
SHITCALL int peHeadChk2(IMAGE_NT_HEADERS64* inh, u32 e_lfanew);
SHITCALL int peHeadChkRva(IMAGE_NT_HEADERS64* inh, u32 rva, u32 len);
SHITCALL int peHeadFinalize(IMAGE_NT_HEADERS64* inh);


static inline 
bool peHead64(IMAGE_NT_HEADERS64* inh) {
	return u8(inh->OptionalHeader.Magic>>8) == 2; };
	
static inline
DWORD peHead_sectAlign(IMAGE_NT_HEADERS64* inh, DWORD v) {
	return ALIGN(v, inh->OptionalHeader.SectionAlignment-1); }
static inline
DWORD peHead_fileAlign(IMAGE_NT_HEADERS64* inh, DWORD v) {
	return ALIGN(v, inh->OptionalHeader.FileAlignment-1); }
	
static inline
u64 peHead_imageBase(IMAGE_NT_HEADERS64* inh) {
	return peHead64(inh) ?
		((IMAGE_NT_HEADERS64*)inh)->OptionalHeader.ImageBase:
		((IMAGE_NT_HEADERS32*)inh)->OptionalHeader.ImageBase;
}
	
static inline 
xarray<PeDataDir> peHeadDataDir(IMAGE_NT_HEADERS64* inh) 
{
	void* dd = peHead64(inh) ?
		&((IMAGE_NT_HEADERS64*)inh)->OptionalHeader.DataDirectory:
		&((IMAGE_NT_HEADERS32*)inh)->OptionalHeader.DataDirectory;
	return {(PeDataDir*)dd, RI(dd,-4)};
}

static inline
xarray<IMAGE_SECTION_HEADER> peHeadSect(IMAGE_NT_HEADERS64* inh) 
{
	u32 sectOfs = offsetof(IMAGE_NT_HEADERS, OptionalHeader);
	sectOfs += inh->FileHeader.SizeOfOptionalHeader;
	return {(IMAGE_SECTION_HEADER*) Void(inh, 
		sectOfs), inh->FileHeader.NumberOfSections};
}

static inline
int peHeadSkip(IMAGE_NT_HEADERS64* inh)
{
	IMAGE_OPTIONAL_HEADER64* ioh = &inh->OptionalHeader;
	return peHead_fileAlign(inh,inh->OptionalHeader.
		SizeOfHeaders)-inh->OptionalHeader.SizeOfHeaders;
}

static inline
int peHeadSize(bool PE64, int nSects)
{
	return PE64 ? sizeof(IMAGE_NT_HEADERS64) 
		: sizeof(IMAGE_NT_HEADERS32) 
		+ sizeof(IMAGE_SECTION_HEADER)*nSects;
}

struct PeHeadWr
{
	byte* data; 
	IMAGE_NT_HEADERS64* inh;
	u32 size, boundImpOfs;
	
	operator IMAGE_NT_HEADERS64*()  { return inh; }
	IMAGE_NT_HEADERS64* operator->(){ return inh; }
	
	
	
	~PeHeadWr() { free(data); }

	PeHeadWr(bool PE64, u32 nSects, xarray<byte> dosHeadr,
		u32 boundImpSz, u32 fileAlign)
	{
		boundImpOfs = peHeadSize(PE64, nSects) + dosHeadr.len;
		size = ALIGN(boundImpOfs+boundImpSz, fileAlign-1);	
		data = xcalloc(size);
		
		inh = memcpyX(data, dosHeadr.data, dosHeadr.len);
		inh->OptionalHeader.SizeOfHeaders = size;
	}
};
