#include "stdafx.h"
#include "peFile.h"

bool peFile_rebaseRsrc(byte* data, u32 size,
	int delta, u32 irdIdx);

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
	SCOPE_EXIT(if(rsrcSect) peFile_rebaseRsrc(rsrcSect->data, 
		rsrcSect->len, -rsrcSect->baseRva, 0));
	ddTmp = {0,0};	
	if(rsrcSect != NULL) { peFile_rebaseRsrc(rsrcSect->data, 
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
	PeHeadWr inh(imageData, PE64(), sects.len, dosHeadr,
		0, ioh().FileAlignment);
	inh->FileHeader.NumberOfSections = sects.len;
	IMAGE_SECTION_HEADER* ish = peHeadSect(inh);
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

int PeFile::Section::resize(PeFile* This, u32 sz)
{
	assert(This->mappMode == false);
	
	u32 allocSize2 = This->sectAlign(sz);
	void* ptr = xrealloc(data, allocSize2);
	u32 base = min(len, sz); len = sz;
	memset(ptr+base, 0, allocSize2-base);
	return allocSize2-::release(allocSize, allocSize2);
}

void PeFile::sectResize(Section* sect, u32 size)
{
	if(sect == NULL) return;
	int delta = sect->resize(this, size);
	if(++sect >= sects.end()) return;
	
	peFile_adjustDataDir(inh, sect->baseRva, delta);

	for(; sect < sects.end(); sect++)
		sect->baseRva += delta;
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


void PeFile::Section::updateType(int type)
{
	Characteristics = and_or(
		Characteristics, getType(type),
		IMAGE_SCN_CNT_UNINITIALIZED_DATA|
		IMAGE_SCN_MEM_DISCARDABLE
	);
}


int PeFile::sectCreate2(cch* name, int type)
{
	DWORD ch = Section::getType(type);
	return sectCreate(name, ch);
}

void PeFile::setRes(void* data, DWORD size)
{
	if(rsrcSect == NULL)
		sectCreate(".rsrc", 0x40000040);
	sectResize(rsrcSect, size);
	memcpy(rsrcSect->data, data, size);
}
