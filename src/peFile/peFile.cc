#include "stdafx.h"


#define ALLOC_LIMIT 1024*1024*128
#define MIN_HEADER_SIZE 0x400
#define MAX_HEADER_SIZE 0x1000
#include "../exe_mod.h"
#include "peFile.h"
#include <Imagehlp.h>

namespace PeFile {
#include "freeBlock.cpp"
#include "peFileInt.h"
#include "peResize.cpp"
#include "peReloc.cpp"
#include "peExport.cpp"
#include "peImport.cpp"
#include "peBlock.cpp"

Void patchChk(int addr, int len)
{
	DWORD rva = addrToRva(addr);
	if(getSection(rva, len))
		return rvaToPtr(rva);
	return NULL;
}

PeSection* getSection(int rva, int len)
{
	for(int i = 0; i < nSections(); i++)
	  if(( peSects[i].baseRva() <= rva)
	  &&( peSects[i].endRva() >= (rva+len)))
		return &peSects[i];
	return NULL;
}

int PeSection::type(void)
{
	if(!strcmp(name(), ".idata"))
		return Type_RData;
	int type = Type_RData;
	if( Characteristics & IMAGE_SCN_MEM_EXECUTE )
		type = Type_Text;
	ei( Characteristics & IMAGE_SCN_MEM_WRITE )
		type = Type_Data;
	return type;
}

int PeSection::remain(void)
{
	DWORD tmp = vSize() % 4096;
	return (4096 - tmp) % 4096;
}

void clearSpace(int rva, int length, int offset)
{
	assert(checkRva(rva, length));
	Relocs_Remove(rva, length);
	memset(rvaToPtr(rva), 0, length);
	if((length -= offset) > 0)
		freeList.mark(rvaToPtr(rva+offset), length, Type_Text);
}

bool RangeChk::check(Void ptr, int length) {
	return ((ptr >= basePtr)&&(ptr+length <= endPtr)); }
int RangeChk::checkStr(Void str) {
	if(str < basePtr) return -1;
	for(int i = 0;; i++) {
	  if(str+i >= endPtr) return -1;
	  if(str[i] == 0) return i+1; } 
}

bool checkHeadr(void)
{
	// check header size
	IMAGE_DOS_HEADER* dosHeadr = imageData;
	if((dosHeadr->e_lfanew + offsetof(IMAGE_NT_HEADERS32,
		OptionalHeader.DataDirectory[16])) > MIN_HEADER_SIZE)
	  return false;
	  
	// update pointers
	IMAGE_NT_HEADERS32* peHeadr = 
		imageData + dosHeadr->e_lfanew;
	fileHeadr = &peHeadr->FileHeader;
	optHeadr = &peHeadr->OptionalHeader;
	dataDir = optHeadr->DataDirectory;
	peSects = Void(optHeadr) +
		fileHeadr->SizeOfOptionalHeader;
	return true;
}

int peRealloc(DWORD delta)
{
	// allocate memory
	Void oldImageData = imageData;
	xrealloc(imageData, imageSize+delta);
	memset(imageData+imageSize, 0, delta);
	imageSize += delta;
	if(oldImageData == NULL)
		return 0;

	// update pointers
	checkHeadr();
	size_t allocDelta = imageData-oldImageData;
	for(auto& sect : Range(peSects, nSections()))
		sect.basePtr() += allocDelta;
	if(extndSection) PTRADD(extndSection, allocDelta);
	if(rsrcSection) PTRADD(rsrcSection, allocDelta);
	if(relocSection) PTRADD(relocSection, allocDelta);
	return allocDelta;
}

static
void validateDirectory(PeSection* section, DWORD iDir)
{
	if(( section || dataDir[iDir].Size )
	&& (( section->vSize() != dataDir[iDir].Size)
	|| ( section->baseRva() != dataDir[iDir].VirtualAddress )))
		printf("warning, bad directory\n");
}

const char* load(const char* fileName)
{
	// error handling
	#define ERROR_RETURN(code) { \
		result = #code; \
		goto ERROR_RETURN1; }
	#define ERROR_CHECK(code, func) { \
		if(func) ERROR_RETURN(code); }
	const char* result;
	FILE* fp = NULL;

	// load pe header
	{assert(imageData == NULL);
	fp = xfopen(fileName, "rb");
	if(fp == NULL)
		ERROR_RETURN( Error_FailedToOpen );
	int fileSize = fsize(fp);
	if(fileSize < MIN_HEADER_SIZE)
		ERROR_RETURN( Corrupt_PeFileToSmall );
	peRealloc(MAX_HEADER_SIZE);
	xfread(imageData, 1, MIN_HEADER_SIZE, fp);
	
	// load rest of header
	if( !checkHeadr() )
		ERROR_RETURN( Corrupt_BadHeader );
	int headerSize = optHeadr->SizeOfHeaders;
	if((fileSize < headerSize )
	||((Void(&peSects[nSections()]) - imageData) > headerSize))
		ERROR_RETURN( Corrupt_BadHeader );
	if((headerSize < MIN_HEADER_SIZE)
	||(headerSize > MAX_HEADER_SIZE))
		ERROR_RETURN( Unsupported_HeaderSize );
	xfread(imageData+MIN_HEADER_SIZE, 1, headerSize-MIN_HEADER_SIZE, fp);
	
	// check header size
	if(headerSize != peSects[0].PointerToRawData) {
		printf("warning, incorrect header size\n");
		headerSize = peSects[0].PointerToRawData;
		optHeadr->SizeOfHeaders = headerSize; 
		fseek(fp, headerSize, SEEK_SET); }

	// validate sections
	int virtualPos = MAX_HEADER_SIZE; 	
	for(PeSection& sect : Range(peSects, nSections()))
	{
		if(sect.PointerToRelocations
		|| sect.PointerToLinenumbers
		|| sect.NumberOfRelocations
		|| sect.NumberOfLinenumbers )
			ERROR_RETURN( Corrupt_Section1 );
		if( virtualPos != sect.baseRva() )
			ERROR_RETURN( Unsupported_Layout );
		virtualPos += ALIGN_PAGE(sect.Misc.VirtualSize);
			
		// locate sections of interest
		if(!strcmp(sect.name(), ".rsrc"))
			rsrcSection = &sect;
		ei(!strcmp(sect.name(), ".reloc"))
			relocSection = &sect;
		else {
			if(rsrcSection || relocSection) {
				printf("warning, unsupported layout\n");
			} else {
				extndSection = &sect;
			}
		}
	}

	// validate directory entries
	if(  dataDir[IDE_ARCH].Size
	||  dataDir[IDE_COM_DESC].Size )
		ERROR_RETURN( Unsupported_Directory );
	validateDirectory(rsrcSection, IDE_RESOURCE);
	validateDirectory(relocSection, IDE_BASERELOC);

	printf("%X\n", virtualPos);
	
	// read the sections
	//if(virtualPos > ALLOC_LIMIT)
	//	ERROR_RETURN( Corrupt_TooLarge1 );
	peRealloc(virtualPos);
	virtualPos = MAX_HEADER_SIZE;
	
	int lastRawEnd = headerSize;
	for(auto& sect : Range(peSects, nSections()))
	{
		int vSize = ALIGN_PAGE(sect.Misc.VirtualSize);
		int rBase = sect.PointerToRawData;
		int rSize = sect.SizeOfRawData;
		byte*& basePtr = sect.basePtr().ptr<byte>();
		
		// Validate section
		basePtr = &imageData[virtualPos];
		virtualPos += vSize;
		if((vSize < rSize) /*||(vSize == 0) */)
			ERROR_RETURN( Corrupt_Section2 );

		// Read section
		if((rBase || rSize) == false)
			continue;
		if((rBase != lastRawEnd)
		|| (rBase+rSize > fileSize))
			ERROR_RETURN( Corrupt_Section2 );
		lastRawEnd = rBase+rSize;
		xfread(basePtr, rSize, fp);
	}
	
	// load file extra
	if(fileSize > lastRawEnd)
	{
		extraSize = fileSize-lastRawEnd;
		virtualPos += extraSize;
		if(virtualPos > ALLOC_LIMIT)
			ERROR_RETURN( Corrupt_TooLarge2 );
		fileExtra = xmalloc(extraSize);
		xfread((char*)fileExtra, extraSize, fp);
	}
	
	// process image file
	sectChk = RangeChk(peSects[0].basePtr(),
		extndSection->endPtr());
	if(rsrcSection != NULL) {
		rsrcChk = RangeChk(rsrcSection->basePtr(),
			rsrcSection->endPtr()); }
	ERROR_CHECK( Error_Relocs , Relocs_Load());
	static const char* const impErr[] = { NULL, "Error_Imports1", 
		"Error_Imports2", "Error_Imports3", "Error_Imports4", "Error_Imports5" };
	result = impErr[Imports_Load()];
	if(result != NULL) goto ERROR_RETURN1;
	if(result = Exports_Load())
		goto ERROR_RETURN1;
	
	// cleanup return
	}if(0) {
ERROR_RETURN1:
	close();}
	fclose(fp);
	return result;
}

DWORD calcExtent(Void buff, DWORD length)
{
	while(length 
	&& (*(BYTE*)(buff+length-1) == 0))
		length = length-1;
	return length;
}

DWORD alignDisk(DWORD rva)
{
	DWORD alignBound = optHeadr->FileAlignment-1;
	return ALIGN(rva, alignBound);
}

const char* save(const char* fileName)
{
	assert(imageData != NULL);
	FILE* fp = xfopen(fileName, "wb");
	if(fp == NULL)
		return "Error_FailedToOpen";
	if(Relocs_Save())
		return "Error_Relocs";
		
	// calculate header size
	optHeadr->SizeOfHeaders = alignDisk(
		calcExtent(imageData, 4096));
	DWORD peFilePos = optHeadr->SizeOfHeaders;
	
	// calculate section sizes
	Void* sectionData_ = xMalloc(nSections());
	Void* sectionData = sectionData_;
	PeSection* endSect = &peSects[nSections()];
	for(PeSection* sect = peSects; sect < endSect; sect++)
	{
		*sectionData++ = sect->basePtr();
		sect->SizeOfRawData = alignDisk(calcExtent(
			sect->basePtr(), sect->vSize()));
		sect->basePtr() = NULL;
		if(sect->SizeOfRawData == 0) continue;
		sect->PointerToRawData = peFilePos;
		peFilePos += sect->SizeOfRawData;
	}
	optHeadr->SizeOfImage = ALIGN_PAGE(
		endSect[-1].endRva());
	
	// write data to file
	xfwrite(imageData, 1, optHeadr->SizeOfHeaders, fp);
	sectionData = sectionData_;
	for(PeSection* sect = peSects; sect < endSect; sect++)
		xfwrite(*sectionData++, 1, sect->SizeOfRawData, fp);
	xfwrite(fileExtra, 1, extraSize, fp);
	fclose(fp);
	free(sectionData_);
	close();
	return 0;
}

void close(void)
{
	// free some stuff
	free_ref(imageData);
	free_ref(fileExtra);
	Relocs_Free();
	Exports_Free();
	Imports_Free();
	freeList.~FreeList();
	
	// clear down all vars
	#define CLR_VAR(x) \
		memset(&x, 0, sizeof(x))

	CLR_VAR(freeList);		// struct, destruct
	CLR_VAR(extraSize);		// int
	CLR_VAR(imageSize);		// int
	CLR_VAR(rsrcChk);		// struct
	CLR_VAR(sectChk);		// struct
	CLR_VAR(dataDir);		// ptr, noalloc
	CLR_VAR(optHeadr);		// ptr, noalloc
	CLR_VAR(fileHeadr);		// ptr, noalloc	
	CLR_VAR(relocSection);	// ptr, noalloc
	CLR_VAR(rsrcSection);	// ptr, noalloc
	CLR_VAR(extndSection);	// ptr, noalloc
	CLR_VAR(peSects);		// ptr, noalloc
}
}
