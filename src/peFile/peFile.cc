#include "stdafx.h"
#include "peFile.h"

xarray<byte> file_getx(FILE* fp, int max)
{
	if(fp->_cnt < max) {
		fp->_base = memcpyX(fp->_base, fp->_ptr, fp->_cnt);
		int cnt = fp->_cnt; fp->_bufsiz -= cnt;
		_filbuf(fp); fp->_base -= cnt; fp->_bufsiz += cnt;
		fp->_ptr -= cnt+1; fp->_cnt += cnt+1;
		if(ferror(fp)) errorDiskFail();
	} 
	
	return {(byte*)fp->_ptr, fp->_cnt};
}

xarray<byte> file_mread(FILE* fp, u32 size)
{
	byte* data = malloc(size);
	if(!data) { if(fsize(fp) < size)
		return {0,0}; errorAlloc(); }
	xfread(data, size, fp);
	return {data, size};
}

bool file_xread(FILE* fp, void* data, u32 size)
{
	if(fread(data, size, 1, fp))
		return true;
	if(ferror(fp)) errorDiskFail();
	return false;
}

void file_skip(FILE* fp, int size)
{
	if(size) fseek(fp, size, SEEK_CUR);
}

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
	
static void pe_checkSum(u16& checkSum,
	byte* data, u32 size)
{
	for(int i = 0; i < size; i += 2) {
		u32 tmp = checkSum + RW(data+i);
		checkSum = tmp + (tmp>>16); }
}

int PeFile::save(cch* fileName)
{
	assert(this->mappMode == false);

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
	//if(pdataSect != NULL) { 
	//	pdata.Rebase(pdataSect->baseRva); }
	
	// tmp: reconstruct dos-header
	xarray<byte> dosHeadr = {imageData.data, 
		((IMAGE_DOS_HEADER*)imageData.data)->e_lfanew};

	// initialize header
	PeHeadWr inh(PE64(), sects.len, dosHeadr,
		boundImp.len, FileAlignment);
	inh->FileHeader.NumberOfSections = sects.len;
	IMAGE_SECTION_HEADER* ish = ioh_pack(inh);
	inh.boundImpSet(boundImp);

	// build section headers
	IMAGE_SECTION_HEADER *ish0; 
	ARGKILL(ish0); ish0 = ish; 
	for(auto& sect : sects) 
	{
		strncpy((char*)ish->Name, sect.name, 8);
		ish->Misc.VirtualSize = sect.len;
		ish->VirtualAddress = sect.baseRva;
		ish->Characteristics = sect.Characteristics;
		ish->SizeOfRawData = sect.extent(*this);
		INCP(ish);
	}
	
	u32 filePos = peHeadFinalize(inh);

	// existing symbol table
	if(nSymbols >= 0) {
		inh->FileHeader.PointerToSymbolTable = filePos;
		inh->FileHeader.NumberOfSymbols = nSymbols; }
		
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
	if(!fp) return 1; xfwrite(inh.data, inh.size, fp);
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
	assert(This->mappMode == false);
	
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
	SCOPE_EXIT(fclose(fp));
	
	// read the ms-dos header
	auto data = file_getx(fp, sizeof(IMAGE_DOS_HEADER));
	int dosSize = peMzChk(data.data, data.len);
	if(dosSize <= 0) ERR(Corrupt_BadHeader);
	file_skip(fp, dosSize);

	// check the pe header
	data = file_getx(fp, sizeof(IMAGE_NT_HEADERS64));
	IMAGE_NT_HEADERS64* peHeadr = Void(data.data);
	int headSize = peHeadChk(peHeadr, dosSize, data.len);
	if(headSize <= 0) ERR(Corrupt_BadHeader);

	// read complete header
	rewind(fp); imageData.xcalloc(mappMode ? 
		peHeadr->OptionalHeader.SizeOfImage : headSize);
	if(!file_xread(fp, imageData.data, headSize))
		ERR(Corrupt_BadHeader);
	peHeadr = Void(imageData.data, dosSize);
	if(peHeadChk2(peHeadr, dosSize))
		ERR(Corrupt_BadHeader);

	// unpack the header
	IMAGE_SECTION_HEADER* ish = ioh_unpack(peHeadr);
	if(dataDir(IDE_BOUNDIMP).rva) {
		boundImp.xcopy(Void(peHeadr, dataDir(IDE_BOUNDIMP)
			.rva-dosSize), dataDir(IDE_BOUNDIMP).size); }
			
	// load sections
	sects.xcalloc(peHeadr->FileHeader.NumberOfSections);
	int virtualPos = SectionAlignment;
	for(auto& sect : sects) 
	{
		// validate section
		if(ish->PointerToRelocations || ish->PointerToLinenumbers
		|| ish->NumberOfRelocations ||ish->NumberOfLinenumbers )
			ERR(Unsupported_SectDbgInfo);
			
		// read section info
		strncpy(sect.name, (char*)ish->Name, 8);
		sect.Characteristics = ish->Characteristics;			
		sect.baseRva = ish->VirtualAddress;
			
		// allocate section
		if(mappMode) { sect.noFree = true;
			sect.init(imageData+sect.baseRva, ish->Misc.VirtualSize);
		} else { sect.resize(this, ish->Misc.VirtualSize); }
		
		// read section data
		if(ish->SizeOfRawData) {
			DWORD size = fileAlign(ish->SizeOfRawData);
			if(!file_xread(fp, sect.data, size))
				ERR(Corrupt_Sect3);
		}
		
		ish++;
	}

	// read file extra
	fileExtra.xalloc(fsize(fp));
	xfread(fileExtra.data, 1, fileExtra.len, fp);
	nSymbols = -1;
	if(peHeadr->FileHeader.PointerToSymbolTable) {
		nSymbols = peHeadr->FileHeader.NumberOfSymbols; 
	}
	
	// load relocations
	this->getSections_(); 
	if(relocSect) {
		auto data = dataDirSectChk(relocSect, &dataDir(IDE_BASERELOC), "reloc");
		if(!data || !relocs.Load(data, data.len, PE64())) ERR(Corrupt_Relocs);
		if(!mappMode) sectResize(relocSect, 0);
	}
	
	// load exceptions
	//if(pdataSect) {
	//	auto data = dataDirSectChk(pdataSect, &dataDir(IDE_EXCEPTION), "pdata");
	//	if(!data || !pdata.Load(data, data.len, pdataSect->baseRva)) 
	//		ERR(Corrupt_Pdata);
	//}
	
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
		//if(!strcmp(sect.name, ".pdata")) pdataSect = &sect;
		if(!strcmp(sect.name, ".rsrc")) rsrcSect = &sect;
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
	assert(this->mappMode == false);

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
