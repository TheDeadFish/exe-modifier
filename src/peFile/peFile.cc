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


bool peFile_rebaseRsrc(byte* data, u32 size,
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
		if( entry->DataIsDirectory ) { if(!peFile_rebaseRsrc(data,
			size, delta, entry->OffsetToDirectory)) return false;
		} else { if(entry->OffsetToDirectory+4 > size) return false;
			RI(data+entry->OffsetToDirectory) += delta;
		}
	} return true;
}

PeFile::~PeFile() {}
void PeFile::Free() { this->~PeFile(); ZINIT; }

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
	if(dosSize <= 0) ERR(Corrupt_BadHeader1);
	file_skip(fp, dosSize);

	// check the pe header
	data = file_getx(fp, sizeof(IMAGE_NT_HEADERS64));
	IMAGE_NT_HEADERS64* peHeadr = Void(data.data);
	int headSize = peHeadChk(peHeadr, dosSize, data.len);
	if(headSize <= 0) ERR(Corrupt_BadHeader2);

	// read complete header
	rewind(fp); imageData.xcalloc(mappMode ? 
		peHeadr->OptionalHeader.SizeOfImage : headSize);
	if(!file_xread(fp, imageData.data, headSize))
		ERR(Corrupt_BadHeader3);
	peHeadr = Void(imageData.data, dosSize);
	IFRET(peHeadChk2(peHeadr, dosSize));

	// unpack the header
	this->inh = peHeadr;
	ImageBase = peHead_imageBase(peHeadr);
	IMAGE_SECTION_HEADER* ish = peHeadSect(peHeadr);
	if(dataDir(IDE_BOUNDIMP).rva) {
		boundImp.xcopy(Void(peHeadr, dataDir(IDE_BOUNDIMP)
			.rva-dosSize), dataDir(IDE_BOUNDIMP).size); }
			
	// load sections
	sects.xcalloc(peHeadr->FileHeader.NumberOfSections);
	int virtualPos = ioh().SectionAlignment;
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
		auto data = dataDirSectChk(relocSect, dataDir(IDE_BASERELOC), "reloc");
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
		if(!peFile_rebaseRsrc(rsrcSect->data, rsrcSect->len, 
			-rsrcSect->baseRva, 0)) ERR(Corrupt_Rsrc);
	}
	

	return NULL;
}

xarray<byte> PeFile::dataDirSectChk(
	Section* sect, DataDir dir, cch* name)
{
	if((sect->baseRva != dir.rva)||(sect->len < dir.size)) return {0,0};
	if(sect->len != dir.size) {
		if(calcExtent(sect->data, sect->len) > dir.size) return {0,0};
		fprintf(stderr, "warning: %s section oversized\n", name); } 
	return {sect->data, dir.size};
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


u32 PeFile::sectAlign(u32 value)
{
	return peHead_sectAlign(inh, value);
}

u32 PeFile::fileAlign(u32 value)
{
	return peHead_fileAlign(inh, value);
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

u32 PeFile::addrToRva(u64 addr) {
	addr -= ImageBase;
	if(addr >> 32) return -1;
	return addr;
}

Void PeFile::addrToPtr(u64 addr){
	return rvaToPtr(addr, 0); }
Void PeFile::addrToPtr(u64 addr, u32 len) {
	addr -= ImageBase; if(addr >> 32) return 0;
	return rvaToPtr(addr, len); }


xarray<cch> PeFile::chkStr2(u32 rva)
{
	auto rng = rvaToPtr2(rva,1); byte* base = rng;
	while(1) { if(!rng.chk()) return {0,0};
	if(!rng.fi()) return {(cch*)base, (cch*)rng.data}; }
}

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


DWORD PeFile::Section::getType(int type)
{
	DWORD ch = IMAGE_SCN_MEM_READ;
	if(type & PeSecTyp::Exec) ch |= IMAGE_SCN_MEM_EXECUTE;
	if(type & PeSecTyp::Write) ch |= IMAGE_SCN_MEM_WRITE;
	if(type & PeSecTyp::NoPage) ch |= IMAGE_SCN_MEM_NOT_PAGED;
	if(!(type & PeSecTyp::NoDisc)) ch |= IMAGE_SCN_MEM_DISCARDABLE;
	if(!(type & PeSecTyp::Intd)) { ch |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
	} else { if(type & type & PeSecTyp::Exec) ch |= IMAGE_SCN_CNT_CODE;
		else ch |= IMAGE_SCN_CNT_INITIALIZED_DATA; }
	return ch;
}

PeFile::DataDir PeFile::dataDir(size_t i)
{
	auto dd = peHeadDataDir(inh);
	if(i >= dd.len) return {0,0};
	return bit_cast<DataDir>(dd[i]);
}

bool PeFile::setDataDir(size_t i, DataDir dir)
{
	auto dd = peHeadDataDir(inh);
	if(i >= dd.len) return false;
	dd[i] = bit_cast<PeDataDir>(dir);
	return true;
}
