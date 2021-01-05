#pragma once

enum { IMAGE_SCN_CNT_DATA = 
	IMAGE_SCN_CNT_INITIALIZED_DATA | 
	IMAGE_SCN_CNT_UNINITIALIZED_DATA
};

struct PeDataDir {DWORD rva, size; };

SHITCALL int peMzChk(void* data, u32 size);
SHITCALL int peHeadChk(IMAGE_NT_HEADERS64* inh, u32 e_lfanew, u32 size);
SHITCALL cch* peHeadChk2(IMAGE_NT_HEADERS64* inh, u32 e_lfanew);
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

#if 0
static inline
int peHeadSize(bool PE64, int nSects)
{
	return (PE64 ? sizeof(IMAGE_NT_HEADERS64) 
		: sizeof(IMAGE_NT_HEADERS32))
		+ sizeof(IMAGE_SECTION_HEADER)*nSects;
}
#endif

__thiscall
void peFile_adjustDataDir(
	IMAGE_NT_HEADERS64* inh, u32 rva, u32 delta);
