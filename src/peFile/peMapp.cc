#include <stdshit.h>
#include "peMapp.h"
#include "peFile.h"

xarray<byte> file_getx(FILE* fp, int max);
xarray<byte> file_mread(FILE* fp, u32 size);
bool file_xread(FILE* fp, void* data, u32 size);
void file_skip(FILE* fp, int size);

int PeMapImg::load(cch* fileName)
{
	// load pe header
	FILE* fp = xfopen(fileName, "rb");
	if(fp == NULL) return 1;
	SCOPE_EXIT(fclose(fp));
	
	// read the ms-dos header
	auto x = file_getx(fp, sizeof(IMAGE_DOS_HEADER));
	int dosSize = peMzChk(x.data, x.len);
	if(dosSize <= 0) return 2;
	file_skip(fp, dosSize);
	
	// check the pe header
	x = file_getx(fp, sizeof(IMAGE_NT_HEADERS64));
	IMAGE_NT_HEADERS64* peHeadr = Void(x.data);
	int headSize = peHeadChk(peHeadr, dosSize, x.len);
	if(headSize <= 0) return headSize ? 3 : 2;
	
	// read complete header
	data.len = peHeadr->OptionalHeader.SizeOfImage;
	data.data = (byte*)calloc(data.len,1);
	if(!data.data) return 4; rewind(fp);
	if(!file_xread(fp, data.data, peHead_fileAlign(
		peHeadr, headSize+dosSize))) return 3;
	
	// check complete header
	inh = Void(data.data, dosSize);
	if(peHeadChk2(inh, dosSize)) return 3;
	sects = peHeadSect(inh);
	dataDir = peHeadDataDir(inh);
	
	// load sections
	for(auto& sect : sects) 
	{
		if(sect.SizeOfRawData) {
			DWORD size = fileAlign(sect.SizeOfRawData);
			void* dst = data+sect.VirtualAddress;
			if(!file_xread(fp, dst, size))
				return 3;
		}		
	}
	
	// load symbol table
	if(inh->FileHeader.PointerToSymbolTable) {
		symb.init(file_mread(fp, fsize(fp)));
		if(!symb) return 3;
	}
	
	printf("xxx %X, %X\n", inh, data.data);
	
	return 0;
}

xarray<byte> PeMapImg::dd(int index)
{
	if((dataDir.len > index)&&(dataDir[index].rva))
		return {data+dataDir[index].rva, dataDir[index].size};
	return {0,0};
}

int PeMapImg::load_relocs(PeReloc& relocs)
{
	auto data = dd(PeFile::IDE_BASERELOC);
	if(data && !relocs.Load(data, data.len, PE64()))
		return 4;	
	return 0;
}
