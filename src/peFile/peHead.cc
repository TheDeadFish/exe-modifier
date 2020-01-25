#include "stdafx.h"
#include "peHead.h"

void PeOptHead_::update(IMAGE_SECTION_HEADER* ish)
{
	SizeOfImage += sectAlign(ish->Misc.VirtualSize);
	u32 vSzFA = fileAlign(ish->Misc.VirtualSize);
	if(ish->Characteristics & IMAGE_SCN_CNT_CODE) { SizeOfCode += vSzFA; 
		if(!BaseOfCode) BaseOfCode = ish->VirtualAddress;		
	} ei(ish->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
		SizeOfInitializedData += vSzFA; goto L1;
	} ei(ish->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
		SizeOfUninitializedData += vSzFA; L1:  
		if(!PE64() && !BaseOfData32) BaseOfData32 = ish->VirtualAddress;	
	}
}

TMPL2(T, U) void pack_common(T* __restrict__ dst, U* __restrict__ src)
{
	#define MV(N) dst->N = src->N;
	MV(Magic) MV(MajorLinkerVersion) MV(MinorLinkerVersion) 
	MV(AddressOfEntryPoint) MV(SectionAlignment) MV(FileAlignment) 
	MV(MajorOperatingSystemVersion) MV(MinorOperatingSystemVersion)
	MV(MajorImageVersion) MV(MinorImageVersion) MV(MajorSubsystemVersion)
	MV(MinorSubsystemVersion) MV(Win32VersionValue) MV(CheckSum)
	MV(Subsystem) MV(DllCharacteristics)
}

TMPL2(T, U) PeOptHead_::DataDirX* pack_extra(
	T* __restrict__ dst, U* __restrict__ src)
{
	MV(ImageBase)
	MV(SizeOfStackReserve) MV(SizeOfStackCommit)
	MV(SizeOfHeapReserve) MV(SizeOfHeapCommit)
	
	// c++ is a piece of utter shit
	if constexpr(!std::is_same_v<T,PeOptHead>)
		return Void(&dst->LoaderFlags);
	else return Void(&src->LoaderFlags);
}

void* PeOptHead::ioh_unpack(void* ioh)
{
	pack_common(this, (IMAGE_OPTIONAL_HEADER64*)ioh);
	
	PeOptHead_::DataDirX* dd = PE64() ?
		pack_extra(this, (IMAGE_OPTIONAL_HEADER64*)ioh):
		pack_extra(this, (IMAGE_OPTIONAL_HEADER32*)ioh);
		
	LoaderFlags = dd->LoaderFlags;
	return memcpyY(dataDir_, 
		dd->dataDir, dd->dataDirSize);
}


void* PeOptHead::ioh_pack(void* ioh)
{
	pack_common((IMAGE_OPTIONAL_HEADER64*)ioh, this);
	ARGFIX(*this); 
	
	PeOptHead_::DataDirX* dd = PE64() ?
		pack_extra((IMAGE_OPTIONAL_HEADER64*)ioh, this):
		pack_extra((IMAGE_OPTIONAL_HEADER32*)ioh, this);
	
	dd->LoaderFlags = LoaderFlags;
	dd->dataDirSize = 0x10;
	return memcpyX(dd->dataDir, dataDir_, 16);
}

int peMzChk(void* data, u32 size)
{
	IMAGE_DOS_HEADER* idh = Void(data);
	if((size < 0x1C)||(idh->e_magic != 'ZM')
	||(size < idh->e_lfarlc)) return -1;
	if(idh->e_lfarlc < 0x40) return 0;
	if(idh->e_lfanew <= 0) return -1;
	return idh->e_lfanew;
}

int peHeadChk(IMAGE_NT_HEADERS64* inh, u32 size)
{
	// check header
	if((size < 4)||(inh->Signature != 'EP')) return 0;
	u32 sectOfs = offsetof(IMAGE_NT_HEADERS, OptionalHeader);
	if((size < sectOfs)||(size < (sectOfs += inh->FileHeader.SizeOfOptionalHeader))
	||(inh->FileHeader.SizeOfOptionalHeader < 2)) { ERR: nothing();	return -1; }
	
	// check optional header magic
	u32 dataDirOffset;
	if(inh->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) { 
		dataDirOffset = offsetof(IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory); }
	ei(inh->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) { 
		dataDirOffset = offsetof(IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory); }
	else { goto ERR; }

	// check optional header size
	if(sectOfs < dataDirOffset) goto ERR;
	u32 dataDirSize = sectOfs-dataDirOffset;
	dataDirSize /= sizeof(IMAGE_DATA_DIRECTORY);
	if((RI(inh, dataDirOffset-4) != dataDirSize)
	||(RI(inh, dataDirOffset-4) > 0x10)) goto ERR;
	return sectOfs;
}
