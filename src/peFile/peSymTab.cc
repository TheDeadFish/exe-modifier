#include "stdafx.h"
#include "peSymTab.h"

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

void PeSymTab::Build_t::xwrite(FILE* fp)
{
	xfwrite(symData.data, symData.len, fp);
	xfwrite(strTab.data.data(), strTab.data.dataSize, fp);
}
