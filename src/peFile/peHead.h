#pragma once

struct PeOptHead_
{
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	
	union { struct { 
		DWORD BaseOfData32;
		DWORD ImageBase32; };
		ULONGLONG ImageBase; 
	};
		
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;	
	
	
	struct DataDir {DWORD rva, size; };
	struct DataDirX { DWORD LoaderFlags;
		DWORD dataDirSize; DataDir dataDir[16]; };
		
		
	TMPL(T) struct SizeOf {
		T SizeOfStackReserve, SizeOfStackCommit;
		T SizeOfHeapReserve, SizeOfHeapCommit; };
		
		
	union { struct { SizeOf<ULONGLONG> sizeOf; DataDirX dd; };
		struct { SizeOf<DWORD> sizeOf32; DataDirX dd32; }; };
		
	// rebuild helpers
	bool PE64() { return u8(Magic>>8) == 2; }
	void update(IMAGE_SECTION_HEADER* ish);
	
	
	
	
	u32 fileAlign(u32 value) {
		return ALIGN(value, FileAlignment-1); }
	u32 sectAlign(u32 value) {
	return ALIGN(value, SectionAlignment-1); }
		
};

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
	
	void* ioh_pack(IMAGE_NT_HEADERS64* inh);
	void* ioh_unpack(IMAGE_NT_HEADERS64* inh);
	
	
	
	
	
	
	
};


SHITCALL int peMzChk(void* data, u32 size);
SHITCALL int peHeadChk(IMAGE_NT_HEADERS64* inh, u32 e_lfanew, u32 size);
SHITCALL int peHeadChk2(IMAGE_NT_HEADERS64* inh, u32 e_lfanew);
SHITCALL int peHeadChkRva(IMAGE_NT_HEADERS64* inh, u32 rva, u32 len);



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
xarray<PeOptHead_::DataDir> peHeadDataDir(IMAGE_NT_HEADERS64* inh) 
{
	void* dd = peHead64(inh) ?
		&((IMAGE_NT_HEADERS64*)inh)->OptionalHeader.DataDirectory:
		&((IMAGE_NT_HEADERS32*)inh)->OptionalHeader.DataDirectory;
	return {(PeOptHead_::DataDir*)dd, RI(dd,-4)};
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
