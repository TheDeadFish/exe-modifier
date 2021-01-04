#include "stdafx.h"
#include "peHead.h"

#define FOR_FI(s,r,i, ...) for(size_t i = 0; i < s.size(); i++)  \
	{ auto& r = s[i]; __VA_ARGS__; }

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

TMPL2(T, U) void pack_fhead(T* __restrict__ dst, U* __restrict__ src)
{
	MV(Machine) MV(Characteristics) MV(TimeDateStamp)
}


TMPL2(T, U) PeDataDirX* pack_extra(
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

IMAGE_SECTION_HEADER* PeOptHead::ioh_unpack(IMAGE_NT_HEADERS64* inh)
{
	pack_fhead(this, &inh->FileHeader);
	IMAGE_OPTIONAL_HEADER64* ioh = &inh->OptionalHeader;
	pack_common(this, (IMAGE_OPTIONAL_HEADER64*)ioh);
	
	PeDataDirX* dd = PE64() ?
		pack_extra(this, (IMAGE_OPTIONAL_HEADER64*)ioh):
		pack_extra(this, (IMAGE_OPTIONAL_HEADER32*)ioh);
		
	LoaderFlags = dd->LoaderFlags;
	
	
	
	
	
	//return memcpyY(dataDir_, 
	//	dd->dataDir, dd->dataDirSize);
	return peHeadSect(inh);
}


IMAGE_SECTION_HEADER* PeOptHead::ioh_pack(IMAGE_NT_HEADERS64* inh)
{
	inh->Signature = 'EP';
	pack_fhead(&inh->FileHeader, this);
	IMAGE_OPTIONAL_HEADER64* ioh = &inh->OptionalHeader;
	pack_common((IMAGE_OPTIONAL_HEADER64*)ioh, this);
	ARGFIX(*this); 
	
	PeDataDirX* dd;
	if(PE64()) { 
		inh->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
		dd = pack_extra((IMAGE_OPTIONAL_HEADER64*)ioh, this);
	} else { 
		inh->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
		dd = pack_extra((IMAGE_OPTIONAL_HEADER32*)ioh, this); }
	
	dd->LoaderFlags = LoaderFlags;
	dd->dataDirSize = 0x10;
	//return memcpyX(dd->dataDir, dataDir_, 16);
	
	return peHeadSect(inh); 
	
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

int peHeadChk(IMAGE_NT_HEADERS64* inh, u32 e_lfanew, u32 size)
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
	
	// check SizeOfHeaders
	sectOfs += sizeof(IMAGE_SECTION_HEADER)*inh->FileHeader.NumberOfSections;
	u32 headSize = inh->OptionalHeader.SizeOfHeaders;
	if(headSize > inh->OptionalHeader.SizeOfImage) goto ERR;
	if(headSize < (sectOfs+e_lfanew)) goto ERR;
	return peHead_fileAlign(inh, headSize);
}

int peHeadChkRva(IMAGE_NT_HEADERS64* inh, u32 rva, u32 len)
{
	auto sects = peHeadSect(inh);
	FOR_FI(sects,sect,i, u32 tmp;
		if(ovf_sub(tmp, rva, sect.VirtualAddress)) continue;
		if(ovf_add(tmp, len)) continue; nothing();
		if(tmp <= sect.Misc.VirtualSize) return i;		
	);
	
	return -1;
}

cch* peHeadChk2(IMAGE_NT_HEADERS64* inh, u32 e_lfanew)
{
	// validate sections
	u32 filePos = peHead_fileAlign(inh, inh->OptionalHeader.SizeOfHeaders);
	u32 virtPos = peHead_sectAlign(inh, inh->OptionalHeader.SizeOfHeaders);
	for(auto& sect : peHeadSect(inh)) {
		DWORD vSize = peHead_sectAlign(inh, sect.Misc.VirtualSize);
		if(virtPos != sect.VirtualAddress)
			return "section: VirtualAddress";

		virtPos += vSize;
		if(sect.PointerToRawData) {
			if((filePos != sect.PointerToRawData)
			||(vSize < sect.SizeOfRawData))
				return "section: RawData";
			filePos += peHead_fileAlign(
				inh, sect.SizeOfRawData);
		}
	}
	
	if(virtPos != inh->OptionalHeader.SizeOfImage)
		return "optional header: SizeOfImage";
	
	// validate symbol table
	if((inh->FileHeader.PointerToSymbolTable)
	&&(inh->FileHeader.PointerToSymbolTable != filePos))
		return "FileHeader: PointerToSymbolTable";
	
	// validate entry point
	DWORD aoep = inh->OptionalHeader.AddressOfEntryPoint;
	if((aoep)&&(peHeadChkRva(inh, aoep, 0) < 0))
		return "optional header: AddressOfEntryPoint";

	// validate DataDirectory
	auto dd = peHeadDataDir(inh);
	FOR_FI(dd,d,i, if(d.rva == 0) continue;
		if(i == IMAGE_DIRECTORY_ENTRY_SECURITY)	continue;
		if(i != IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT) {
			if(peHeadChkRva(inh, d.rva, d.size) < 0)
				return "DataDirectory"; }
			
		// validate bound import
		else { u32 tmp;
			if(d.rva < e_lfanew) return "Bound Import";
			if(ovf_add(tmp, d.rva, d.size)||(tmp>inh
			->OptionalHeader.SizeOfHeaders))
					return "Bound Import"; }
	)

	return 0;
}

SHITCALL int peHeadFinalize(IMAGE_NT_HEADERS64* inh)
{
	IMAGE_OPTIONAL_HEADER32* ioh = Void(&inh->OptionalHeader);
	int filePos = ioh->SizeOfHeaders;
	ioh->SizeOfImage = peHead_sectAlign(inh, filePos);

	auto sects = peHeadSect(inh);
	for(auto& sect : sects) {
	
		// update section size
		if(sect.SizeOfRawData) goto HAS_DATA;
		if(sect.Characteristics & 0x60) {
			sect.SizeOfRawData = ioh->FileAlignment;
			HAS_DATA:	sect.PointerToRawData = filePos;
			filePos += sect.SizeOfRawData;
		}
		
		// update optional header
		ioh->SizeOfImage += peHead_sectAlign(inh, sect.Misc.VirtualSize);
		u32 fSzFA = peHead_fileAlign(inh, sect.SizeOfRawData);
		if((sect.Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
		&&(strcmp((char*)sect.Name, "INIT"))) continue;
		
		// SizeOfCode/Data
		if(sect.Characteristics & IMAGE_SCN_CNT_CODE) 
			ioh->SizeOfCode += fSzFA;
		if(sect.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			ioh->SizeOfInitializedData += fSzFA;
		if(sect.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
			u32 vSzFA = peHead_fileAlign(inh, sect.Misc.VirtualSize);
			ioh->SizeOfUninitializedData += vSzFA;
		}
			
		// BaseOfCode/Data
		if(sect.Characteristics & IMAGE_SCN_CNT_CODE) {
			if(!ioh->BaseOfCode) ioh->BaseOfCode = sect.VirtualAddress;	
		} ei(sect.Characteristics & IMAGE_SCN_CNT_DATA) {
			if(!peHead64(inh) && !ioh->BaseOfData) 
				ioh->BaseOfData = sect.VirtualAddress;
		}
	}
		
	return filePos;
}

void peFile_adjustDataDir(
	IMAGE_NT_HEADERS64* inh, u32 rva, u32 delta)
{
	auto dd = peHeadDataDir(inh);
	max_ref(dd.len, IMAGE_NUMBEROF_DIRECTORY_ENTRIES);
	for(auto& dir : dd) { if(dir.rva >= rva) dir.rva += delta; }
}
