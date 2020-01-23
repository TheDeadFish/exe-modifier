#include "stdafx.h"
#include "peFile.h"

SHITSTATIC
bool rebaseRsrc(byte* data, u32 size,
	int delta, u32 irdIdx)
{
	// peform bounds check
	IMAGE_RESOURCE_DIRECTORY* rsrcDir = Void(data,irdIdx);
	IMAGE_RESOURCE_DIRECTORY_ENTRY* entry = Void(rsrcDir+1);
	if(((byte*)entry) > (data+size)) return false;
	IMAGE_RESOURCE_DIRECTORY_ENTRY* lastEnt = entry + rsrcDir->
		NumberOfNamedEntries+rsrcDir->NumberOfIdEntries;
	if(((byte*)lastEnt) > (data+size)) return false;
	
	// process directory entries
	for(; entry < lastEnt; entry++) {
		if( entry->DataIsDirectory ) { if(!rebaseRsrc(data,
			size, delta, entry->OffsetToDirectory)) return false;
		} else { if(entry->OffsetToDirectory+4 > size) return false;
			RI(data+entry->OffsetToDirectory) += delta;
		}
	} return true;
}

bool PeReloc::Load(byte* buff, 
	u32 size, bool PE64)
{
	// determine last relocation block
	IMAGE_BASE_RELOCATION* relocPos = Void(buff);
	IMAGE_BASE_RELOCATION* relocEnd = Void(buff,size-8);
	u32 maxRva = 0; 
	while((relocPos < relocEnd) && relocPos->SizeOfBlock) {
		max_ref(maxRva, relocPos->VirtualAddress);
		PTRADD(relocPos, relocPos->SizeOfBlock); }
		
		
	// read the relocation blocks	
	xncalloc2((maxRva>>12)+1); relocPos = Void(buff);
	while((relocPos < relocEnd) && relocPos->SizeOfBlock) {
		auto& br = data[relocPos->VirtualAddress>>12]; int count = 
		(relocPos->SizeOfBlock-8)/2; br.xreserve(count); relocPos++;
		while(count--) { u32 wReloc = RDI(PW(relocPos));
			u32 type = wReloc / 4096; u32 val = wReloc & 4095;
			if(type != 0) { if(type != (PE64 ? 10 : 3))
				return false; br.ib() = val; }
		}
	}
	
	return true;
}

u32 PeReloc::build_size(void)
{
	u32 totalSize = 0;
	for(auto& br : *this) if(br.size) {
	totalSize += ALIGN4(br.size*2+8); }
	return totalSize;
}

void PeReloc::build(byte* buff, bool PE64)
{
	int type = (PE64 ? 0xA000 : 0x3000);
	IMAGE_BASE_RELOCATION* curPos = Void(buff);
	for(u32 bi = 0; bi < size; bi++) if(data[bi].size) {
		curPos->VirtualAddress = bi<<12; auto& br = data[bi]; 
		curPos->SizeOfBlock = ALIGN4(br.size*2+8); curPos++;
		qsort(br.data, br.size, [](const u16& a, const u16& b) {
			return a-b; }); for(u16 val : br) { WRI(PW(
			curPos), val | type); } if(br.size&1) PW(curPos)++; 
	}
}

bool PeReloc::Find(u32 rva)
{
	u32 block = rva>>12; rva &= 4095;
	if(block < size)
	for(auto val : data[block])
	if(val == rva) return true;
	return false;
}

void PeReloc::Add(u32 rva)
{
	if(Find(rva)) return;
	u32 block = rva>>12; rva &= 4095;
	xncalloc2(block+1);
	data[block].push_back(rva);
}

void PeReloc::Remove(u32 rva) {
	return Remove(rva, 1); }

void PeReloc::Remove(u32 rva, u32 len)
{
	u32 rvaEnd = rva+len;
	for(u32 bi = rva>>12; bi < size; bi++) {
	u32 base = bi<<12; if(base >= rvaEnd) break;
	for(u32 i = 0; i < data[bi].size; i++) {
	  if(inRng1(data[bi][i]+base, rva, rvaEnd))
	    data[bi][i] = data[bi][--data[bi].size]; }
	}
}

void PeReloc::Move(u32 rva, u32 length, int delta)
{
	u32 end = rva + length;
	u32 bi = rva>>12; u32 be = end>>12;
	if(delta > 0) std::swap(bi, be);
	while(1) { xArray<u16> tmp; tmp.init(data[bi].
		data, data[bi].size); data[bi].init();
		u32 base = bi<<12; for(u16 val : tmp) { 
		u32 rlc = val+base; if(inRng(rlc, rva, end)) 
			rlc += delta; this->Add(rlc); }
		if(bi == be) break; bi += delta > 0 ? -1 : 1;
	}
}

void PeSymTab::add(char* name, u32 rva)
{
	symbol.push_back(name, rva);
}


u32 PeSymTab::StrTable::add(char* str)
{
	if(data.dataSize == 0) {
		data.write32(4); }
	u32 size = data.dataSize;
	data.strcat2(str);
	RI(data.dataPtr) = data.dataSize;
	return size;
}

inline
void PeSymTab::Build_t::xwrite(FILE* fp)
{
	xfwrite(symData.data, symData.len, fp);
	xfwrite(strTab.data.data(), strTab.data.dataSize, fp);
}

void PeFile::symTab_build(PeSymTab::Build_t& bd)
{
	for(auto& ss : symtab.symbol) {
	
		// create symbol
		auto& sd = bd.symData.push_back();
		sd.StorageClass = 2;
		
		// initialize the string
		int len = strlen(ss.name);
		if(len <= 8) { strncpy(sd.Name, ss.name, 8);
		} else { sd.Name2 = bd.strTab.add(ss.name); }

		// lookup the section
		Section* sect = rvaToSect(ss.rva,0);
		assert(sect != NULL);
		sd.Section = iSect(sect)+1;
		sd.Value = ss.rva-sect->baseRva;
	}
}

#define ARGKILL(arg) asm("" : "=m"(arg));

// bit-array manipulation
#define rBTST(v,i)({bool rbrt_;asm("bt %1, %k2;":"=@ccc"(rbrt_):"ir"(i),"g"(v));rbrt_;})
#define rBINV(v,i)({bool rbrt_;asm("btc %2, %k1;":"=@ccc"(rbrt_),"+g"(v):"ir"(i));rbrt_;})
#define rBRST(v,i)({bool rbrt_;asm("btr %2, %k1;":"=@ccc"(rbrt_),"+g"(v):"ir"(i));rbrt_;})
#define pBSET(v,i)({bool rbrt_;asm("bts %2, %k1;":"=@ccc"(rbrt_),"+g"(v):"ir"(i));rbrt_;})

// IMAGE_OPTIONAL_HEADER packing
const char ioh_cpySkipTab[] = {2, -12, 4, -8, 65, 24, -8, 8, 68, 4, 0};
#define IOH_PACKUNPACK(src, dst, sa, da) byte* dstPos = dst;\
	REGFIX(D,dstPos); byte* srcPos = src; REGFIX(S,srcPos);  \
	{ cch* cptPos = ioh_cpySkipTab; REGFIX(a,cptPos); \
	while(int ch = RDI(cptPos)) { if(ch < 0) { sa -= ch; } \
	ei(!rBRST(ch, 6)) { MOVSNX_(dstPos, srcPos, ch, 1); } else { \
	if(ch == 1) { if(PE64()) sa -= 4; } \
	do { MOVSN_(dstPos, srcPos, 4); if(PE64()) { MOVSN_(dstPos, \
	srcPos, 4);}else{da += 4; nothing();}}while(--ch > 0);}}} \
	
static void pe_checkSum(u16& checkSum,
	byte* data, u32 size)
{
	for(int i = 0; i < size; i += 2) {
		u32 tmp = checkSum + RW(data+i);
		checkSum = tmp + (tmp>>16); }
}

int PeFile::save(cch* fileName)
{
	// rebuild relocs
	SCOPE_EXIT(sectResize(relocSect, 0));
	DataDir ddTmp = {0,0};
	if(relocSect != NULL) { sectResize(
		relocSect, relocs.build_size());
		relocs.build(relocSect->data, PE64());
		ddTmp = relocSect->dataDir();
	} dataDir(IDE_BASERELOC) = ddTmp;
	
	// rebase resources
	SCOPE_EXIT(if(rsrcSect) rebaseRsrc(rsrcSect->data, 
		rsrcSect->len, -rsrcSect->baseRva, 0));
	ddTmp = {0,0};	
	if(rsrcSect != NULL) { rebaseRsrc(rsrcSect->data, 
		rsrcSect->len, rsrcSect->baseRva, 0); 
		ddTmp = rsrcSect->dataDir();
	} dataDir(IDE_RESOURCE) = ddTmp;
	
	// rebase exceptions
	if(pdataSect != NULL) { 
		pdata.Rebase(pdataSect->baseRva); }

	// allocate header buffer
	u32 inhSize = PE64() ? sizeof(IMAGE_NT_HEADERS64)
		: sizeof(IMAGE_NT_HEADERS32);
	u32 headrSize = fileAlign(dosHeadr.len+ inhSize + 
		sizeof(IMAGE_SECTION_HEADER)*sects.len + boundImp.len);
	byte* headrBuff = xcalloc(headrSize);
	SCOPE_EXIT(free(headrBuff));
	IMAGE_NT_HEADERS32* inh = memcpyX(headrBuff,
		dosHeadr.data, dosHeadr.len);
	inh->OptionalHeader.SizeOfHeaders = headrSize;
	inh->OptionalHeader.SizeOfImage = SectionAlignment;

	// unpack headers
	inh->Signature = 'EP'; memcpy(&inh->FileHeader,
		&ifh, sizeof(IMAGE_FILE_HEADER));
	inh->FileHeader.NumberOfSections = sects.len;
	inh->FileHeader.SizeOfOptionalHeader = PE64() ? sizeof(
	IMAGE_OPTIONAL_HEADER64) : sizeof(IMAGE_OPTIONAL_HEADER32);
	inh->OptionalHeader.Magic = PE64() ? 0x20b : 0x10b;
	IOH_PACKUNPACK(&MajorLinkerVersion, &inh->OptionalHeader.
		MajorLinkerVersion, dstPos, srcPos);
	WRI(PI(dstPos), 0x10); dstPos = memcpyX(
		dstPos, dataDir(), sizeof(dataDir()));

	// build section headers
	IMAGE_SECTION_HEADER *ish0, *ish = Void(dstPos);
	ARGKILL(ish0); ish0 = ish; 
	u32 filePos = headrSize;
	for(auto& sect : sects) 
	{
		// build section header
		strncpy((char*)ish->Name, sect.name, 8);
		ish->Misc.VirtualSize = sect.len;
		ish->VirtualAddress = sect.baseRva;
		ish->Characteristics = sect.Characteristics;
		if(sect.data) { ish->PointerToRawData = filePos;
		filePos += (ish->SizeOfRawData = sect.extent(*this)); }
		
		// update IMAGE_OPTIONAL_HEADER
		u32 vSzFA = fileAlign(ish->Misc.VirtualSize);
		if(ish->Characteristics & IMAGE_SCN_CNT_CODE) {
			inh->OptionalHeader.SizeOfCode += vSzFA;
			if(!inh->OptionalHeader.BaseOfCode) inh->OptionalHeader
			.BaseOfCode = ish->VirtualAddress; } else {
		if(ish->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			inh->OptionalHeader.SizeOfInitializedData += vSzFA;
		ei(ish->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			inh->OptionalHeader.SizeOfUninitializedData += vSzFA;
		else { goto L1; } if(!PE64() && !inh->OptionalHeader.BaseOfData)
			inh->OptionalHeader.BaseOfData = ish->VirtualAddress; L1:; } 
		inh->OptionalHeader.SizeOfImage += sect.allocSize; INCP(ish);
	}
	
	// write bound import
	IMAGE_DATA_DIRECTORY* idd = Void(ish0, -128);
	idd[IDE_BOUNDIMP].Size = boundImp.len;
	idd[IDE_BOUNDIMP].VirtualAddress = 0;
	if(boundImp.data) { idd[IDE_BOUNDIMP].
		VirtualAddress = PTRDIFF(ish, headrBuff);
		memcpyX((byte*)ish, boundImp.data, boundImp.len); }
		
	// build symbol table
	PeSymTab::Build_t symData;
	symTab_build(symData);
	if(symData.hasData()) {
		inh->FileHeader.PointerToSymbolTable = filePos;
		inh->FileHeader.NumberOfSymbols = symData.symData.len;
	}
	
	// calculate checksum
	//u16 checkSum = 0; pe_checkSum(checkSum, 
	//	headrBuff, headrSize); ish = Void(ish0);
	//for(auto& sect : sects) { if(sect.data) { pe_checkSum(
	//	checkSum, sect.data, ish->SizeOfRawData); } ish++; }
	//pe_checkSum(checkSum, fileExtra.data, fileExtra.size);
	//inh->OptionalHeader.CheckSum = checkSum + filePos + fileExtra.size;
	
	// write sections
	FILE* fp = xfopen(fileName, "wb");
	if(!fp) return 1; xfwrite(headrBuff, headrSize, fp);
	ish = Void(ish0);	for(auto& sect : sects) { if(sect.data) {
		xfwrite(sect.data, ish->SizeOfRawData, fp); } ish++; }
	symData.xwrite(fp);
	xfwrite(fileExtra.data, fileExtra.len, fp);
	
	
	
	
	fclose(fp); return 0;
}

PeFile::~PeFile() {}
void PeFile::Free() { this->~PeFile(); ZINIT; }



int PeFile::Section::resize(PeFile* This, u32 sz)
{
	u32 allocSize2 = This->sectAlign(sz);
	void* ptr = xrealloc(data, allocSize2);
	u32 base = min(len, sz); len = sz;
	memset(ptr+base, 0, allocSize2-base);
	return allocSize2-::release(allocSize, allocSize2);
}

cch* PeFile::load(cch* fileName)
{
	#define ERR(x) { Free(); return #x; }

	// load pe header
	FILE* fp = xfopen(fileName, "rb");
	if(fp == NULL) ERR(Error_FailedToOpen);
	int fileSize = min(fsize(fp), 0x1000);
	if(fileSize < sizeof(IMAGE_DOS_HEADER))
		ERR(Corrupt_BadHeader);
	Void header = xmalloc(fileSize);
	SCOPE_EXIT(free(header));
	xfread(header, 1, fileSize, fp);
	Void headEnd = header+fileSize;
	
	// read the ms-dos header
	IMAGE_DOS_HEADER* idh = header;
	IMAGE_NT_HEADERS64* peHeadr = header+idh->e_lfanew;
	if(((char*)peHeadr->OptionalHeader.DataDirectory) > headEnd)
		ERR(Corrupt_BadHeader);
	dosHeadr.xcopy(header, idh->e_lfanew);
	
	// read IMAGE_FILE_HEADER
	if((peHeadr->Signature != 'EP')||(peHeadr->FileHeader.
	SizeOfOptionalHeader > sizeof(IMAGE_OPTIONAL_HEADER64)))
		ERR(Corrupt_BadHeader);
	//if(peHeadr->FileHeader.PointerToSymbolTable)
	//	ERR(Unsupported_SymbolTable);
	memcpy(&ifh, &peHeadr->FileHeader, sizeof(IMAGE_FILE_HEADER));
	
	// read IMAGE_OPTIONAL_HEADER
	Magic = peHeadr->OptionalHeader.Magic;
	if(!is_one_of(Magic, 0x10b, 0x20b)) ERR(Corrupt_BadHeader);
	IOH_PACKUNPACK(&peHeadr->OptionalHeader.MajorLinkerVersion, 
		&MajorLinkerVersion, srcPos, dstPos);
	
	// read IMAGE_DATA_DIRECTORY
	DWORD NumberOfRvaAndSizes = RDI(PI(srcPos));
	IMAGE_DATA_DIRECTORY* idd = Void(srcPos);
	if((NumberOfRvaAndSizes > IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
	||(&idd[NumberOfRvaAndSizes] > headEnd)) ERR(Corrupt_BadHeader); 
	memcpyX(&dataDir()[0], idd, NumberOfRvaAndSizes);
	
	// check IMAGE_SECTION_HEADER
	IMAGE_SECTION_HEADER* ish = Void(&peHeadr->OptionalHeader, 
		peHeadr->FileHeader.SizeOfOptionalHeader);
	int NumberOfSections = peHeadr->FileHeader.NumberOfSections;
	if(ish+NumberOfSections > headEnd) ERR(Corrupt_BadHeader);
	
	// read bound import
	if(dataDir(IDE_BOUNDIMP).rva) {
		byte* data = header+dataDir(IDE_BOUNDIMP).rva;
		boundImp.xcopy(data, dataDir(IDE_BOUNDIMP).size); }
	
	// load sections
	sects.xcalloc(NumberOfSections);
	int virtualPos = SectionAlignment;
	for(auto& sect : sects) 
	{
		// validate section
		if(ish->PointerToRelocations || ish->PointerToLinenumbers
		|| ish->NumberOfRelocations ||ish->NumberOfLinenumbers )
			ERR(Unsupported_SectDbgInfo);
		if(virtualPos != ish->VirtualAddress) ERR(Corrupt_Sect1);
		sect.baseRva = virtualPos; virtualPos += 
			sect.resize(this, ish->Misc.VirtualSize);
		
		// read section data
		strncpy(sect.name, (char*)ish->Name, 8);
		sect.Characteristics = ish->Characteristics;
		if(ish->PointerToRawData && ish->SizeOfRawData) {
		if(ish->SizeOfRawData > sect.allocSize)
			ERR(Corrupt_Sect2);
		fseek(fp, ish->PointerToRawData, SEEK_SET);
		if(!fread(sect.data, ish->SizeOfRawData, 1, fp)) {
			if(ferror(fp)) errorDiskFail();
			else ERR(Corrupt_Sect3); }
		}
		
		ish++;
	}
	
	// read file extra
	fileExtra.xalloc(fsize(fp));
	xfread(fileExtra.data, 1, fileExtra.len, fp);
	
	// load relocations
	this->getSections_(); 
	if(relocSect) {
		auto data = dataDirSectChk(relocSect, &dataDir(IDE_BASERELOC), "reloc");
		if(!data || !relocs.Load(data, data.len, PE64())) ERR(Corrupt_Relocs);
		sectResize(relocSect, 0);
	}
	
	// load exceptions
	if(pdataSect) {
		auto data = dataDirSectChk(pdataSect, &dataDir(IDE_EXCEPTION), "pdata");
		if(!data || !pdata.Load(data, data.len, pdataSect->baseRva)) 
			ERR(Corrupt_Pdata);
	}
	
	if(rsrcSect) {
		if(!rebaseRsrc(rsrcSect->data, rsrcSect->len, 
			-rsrcSect->baseRva, 0)) ERR(Corrupt_Rsrc);
	}
	

	return NULL;
}

xarray<byte> PeFile::dataDirSectChk(
	Section* sect, DataDir* dir, cch* name)
{
	if((sect->baseRva != dir->rva)||(sect->len < dir->size)) return {0,0};
	if(sect->len != dir->size) {
		if(calcExtent(sect->data, sect->len) > dir->size) return {0,0};
		fprintf(stderr, "warning: %s section oversized\n", name); } 
	return {sect->data, dir->size};
}

void PeFile::getSections_(void)
{
	rsrcSect = NULL; relocSect = NULL;
	for(auto& sect : sects) {
		if(!strcmp(sect.name, ".pdata")) pdataSect = &sect;
		ei(!strcmp(sect.name, ".rsrc")) rsrcSect = &sect;
		ei(!strcmp(sect.name, ".reloc")) relocSect = &sect;
		ei((strcmp(sect.name, ".debug") && !rsrcSect
			&& !relocSect)) extendSect = &sect;	
	}
}

void PeFile::sectResize(Section* sect, u32 size)
{
	if(sect == NULL) return;
	int delta = sect->resize(this, size);
	if(++sect >= sects.end()) return;
	for(auto& dir : dataDir()) if(dir.rva >=
		sect->baseRva) dir.rva += delta; 
	for(; sect < sects.end(); sect++)
		sect->baseRva += delta;
}


#if 0
int main()
{
	PeFile peFile;
	cch* ret = peFile.load("user32.dll");
	printf("!!%s\n", ret);
	
	printf("%X\n", peFile.MajorLinkerVersion);
	printf("%X\n", peFile.MinorLinkerVersion);
	printf("%X\n", peFile.AddressOfEntryPoint);
	printf("%I64X\n", peFile.ImageBase);
	printf("%X\n", peFile.SectionAlignment);
	printf("%X\n", peFile.FileAlignment);
	printf("%X\n", peFile.MajorOperatingSystemVersion);
	printf("%X\n", peFile.MinorOperatingSystemVersion);
	printf("%X\n", peFile.MajorImageVersion);
	printf("%X\n", peFile.MinorImageVersion);
	printf("%X\n", peFile.MajorSubsystemVersion);
	printf("%X\n", peFile.MinorSubsystemVersion);
	printf("%X\n", peFile.Win32VersionValue);
	printf("%X\n", peFile.Subsystem);
	printf("%I64X\n", peFile.SizeOfStackReserve);
	printf("%I64X\n", peFile.SizeOfStackCommit);
	printf("%I64X\n", peFile.SizeOfHeapReserve);
	printf("%I64X\n", peFile.SizeOfHeapCommit);
	printf("%X\n\n", peFile.LoaderFlags);
	
	for(int i = 0; i < 16; i++) {
		printf("%X, %X\n", peFile.dataDir[i].rva,
			peFile.dataDir[i].size); }
	
	
	
	peFile.save("out.exe");






}

#endif

u32 PeFile::sectAlign(u32 value)
{
	return ALIGN(value, SectionAlignment-1);
}

u32 PeFile::fileAlign(u32 value)
{
	return ALIGN(value, FileAlignment-1);
}

DWORD PeFile::calcExtent(Void buff, DWORD length)
{
	if(buff) while(length
	&& (*(BYTE*)(buff+length-1) == 0))
		length = length-1;
	return length;
}


PeFile::Section* PeFile::ptrToSect(void* ptr, u32 len) {
	for(auto& sect : sects) { if((sect.data <= (byte*)ptr)
	  &&( sect.end() >= (byte*)ptr+len)) return &sect; } return 0; }
PeFile::Section* PeFile::rvaToSect(u32 rva, u32 len) {
	FOR_REV(auto& sect, sects, if(( sect.baseRva <= rva ) 
	  &&( sect.endRva() >= (rva+len))) return &sect; ) return 0; }
	  
Void PeFile::patchChk(u64 addr, u32 len) {
	 if((addr-=ImageBase) > 0xFFFFFFFF) return 0;
	return rvaToPtr(addr, len); }
Void PeFile::rvaToPtr(u32 rva) {
	return rvaToPtr(rva, 0); }
Void PeFile::rvaToPtr(u32 rva, u32 len) {
	return rvaToPtr2(rva, len).data; }
xRngPtr<byte> PeFile::rvaToPtr2(u32 rva, u32 len) {
	auto* sect = rvaToSect(rva, len); if(!sect) 
	return {0,0}; return {sect->rvaPtr(rva), sect->end()};  }
	
u32 PeFile::ptrToRva(void* ptr) {
	return ptrToRva(ptr, 0); }
u32 PeFile::ptrToRva(void* ptr, u32 len) {
	auto* sect = ptrToSect(ptr, len); if(!sect)
	return 0; return sect->ptrRva(ptr); }

xarray<cch> PeFile::chkStr2(u32 rva)
{
	auto rng = rvaToPtr2(rva,1); byte* base = rng;
	while(1) { if(!rng.chk()) return {0,0};
	if(!rng.fi()) return {(cch*)base, (cch*)rng.data}; }
}

/*
bool PeFile::Section::normSect(cch* name)
{
	static cch* names[] = {
	".text", ".rdata", ".data", ".bss",
	"INIT", "PAGE", ".idata", ".CRT", ".tls" };
	for(cch* nm) if(strScmp(name, nm)) return true;
	return false;
}*/

int PeFile::Section::type(void)
{
	return type(Characteristics);
}

int PeFile::Section::type(DWORD ch)
{
	if((!(ch & IMAGE_SCN_MEM_READ))
	||(ch & 0x150FFF1F)) return PeSecTyp::Weird;
	int type = 0;
	if(ch & (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_CODE))
		type |= PeSecTyp::Intd;
	if(ch & IMAGE_SCN_MEM_EXECUTE) type |= PeSecTyp::Exec;
	if(ch & IMAGE_SCN_MEM_WRITE) type |= PeSecTyp::Write;
	if(ch & IMAGE_SCN_MEM_NOT_PAGED) type |= PeSecTyp::NoPage;
	if(!(ch & IMAGE_SCN_MEM_DISCARDABLE)) type |= PeSecTyp::NoDisc;
	return type;
}

u32 PeFile::Section::extent(PeFile& peFile)
{
	return peFile.fileAlign(peFile.
		calcExtent(data, len));
}

int PeFile::sectCreate(cch* name, DWORD ch)
{
	// perform the insertion
	int insIdx = iSect2(extendSect)+1;
	sects.xresize(sects.len+1); 
	memmove(sects.data+insIdx+1, sects.data+insIdx,
		(sects.len-(insIdx+1))*sizeof(Section));
	
	// initialize section
	memset(sects+insIdx, 0, sizeof(Section));
	strcpy(sects[insIdx].name, name);
	sects[insIdx].Characteristics = ch;
	if(insIdx > 0) sects[insIdx].baseRva
		= sects[insIdx-1].endPage();
	getSections_(); return insIdx;
}

int PeFile::sectCreate2(cch* name, int type)
{
	DWORD ch = IMAGE_SCN_MEM_READ;
	if(type & PeSecTyp::Exec) ch |= IMAGE_SCN_MEM_EXECUTE;
	if(type & PeSecTyp::Write) ch |= IMAGE_SCN_MEM_WRITE;
	if(type & PeSecTyp::NoPage) ch |= IMAGE_SCN_MEM_NOT_PAGED;
	if(!(type & PeSecTyp::NoDisc)) ch |= IMAGE_SCN_MEM_DISCARDABLE;
	if(!(type & PeSecTyp::Intd)) { ch |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
	} else { if(type & type & PeSecTyp::Exec) ch |= IMAGE_SCN_CNT_CODE;
		else ch |= IMAGE_SCN_CNT_INITIALIZED_DATA; }
		
	return sectCreate(name, ch);
}

void PeFile::setRes(void* data, DWORD size)
{
	if(rsrcSect == NULL)
		sectCreate(".rsrc", 0x40000040);
	sectResize(rsrcSect, size);
	memcpy(rsrcSect->data, data, size);
}


bool PeExcept::Load(byte* data, u32 size, u32 rva)
{
	funcs.init((RtFunc*)data, size / sizeof(RtFunc));
	for(auto& fn : funcs) { if(fn.addr & 1) {
			u32 idx = (fn.addr-rva) / sizeof(RtFunc);
			if(idx >= funcs.len) return false;
			fn.addr = idx | INT_MIN; } 
	} return true;
}

void PeExcept::Rebase(u32 rva)
{
	rva += 1;
	for(auto& fn : funcs) {
		if(isNeg(fn.addr))
			fn.addr = (fn.addr*12)+rva;
	}
}

PeExcept::RtFunc* PeExcept::find(u32 rva, u32 len)
{
	u32 end = (rva+len)-1;
	for(auto& rtf : funcs) { 
		if((rtf.start <= rva) &&(rtf.end >= end))	
			if(rtf.addr) return &rtf; break; }
	return NULL;
}

void PeExcept::kill(u32 rva, u32 len) 
{
	RtFunc* prtf = find(rva, len);
	if(prtf) prtf->addr = 0;
}
