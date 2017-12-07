#include "stdafx.h"
#include "../exe_mod.h"
#include "linker.h"
#include <conio.h>
namespace Linker {
#include "library.cpp"
#include "object.cpp"
#include "relocs.cpp"
#include "imports.cpp"
#include "exports.cpp"
#include "gc-sections.cpp"

Section* sections;
DWORD nSections;
Symbol* symbols;
DWORD nSymbols;
Reloc* relocs;
DWORD nRelocs;
xarray<char*> keep_list;

const char* nullchk(const char* str, const char* limit)
{
	while(str < limit)
	  if(*str++ == '\0')
		return str;
	return NULL;
}

DWORD addSection(const char* fileName, const char* Name,
	void* rawData, WORD type, WORD align,
		DWORD baseRva, DWORD length)
{
	auto& sect = xNextAlloc(sections, nSections);
	sect.fileName = fileName;
	sect.name = xstrdup(Name);
	sect.rawData = xcalloc(length);
	if(rawData != NULL)
		memcpy(sect.rawData, rawData, length);
	sect.relocs = 0; sect.nReloc = 0;
	sect.type = type; sect.align = align;
	sect.baseRva = baseRva; sect.length = length; 
	return nSections-1;
};

void destroy_section(Section& section)
{
	free_ref(section.relocs);
	free_ref(section.rawData);
	section.name[0] = '\0';
	section.type = -1;
	section.length = 0;
	section.nReloc = 0;	
}

Section* findSection(const char* name)
{
	for(int i = 0; i < nSections; i++)
	  if((sections[i].name != NULL)
	  &&(!strcmp(sections[i].name, name)))
		return &sections[i];
	return NULL;
}

int findSymbol(const char* Name)
{
	if(Name != NULL)
	  for(int i = 0; i < nSymbols; i++)
		if((symbols[i].Name != NULL)
		&&(!strcmp(symbols[i].Name, Name)))
		  return i;
	return -1;
}

int addSymbol(const char* Name, DWORD section, DWORD weakSym, DWORD value)
{
	// allocate new symbol ?
	int symIndex = findSymbol(Name);
	if( symIndex < 0 ) {
		Symbol& symb = xNextAlloc(symbols, nSymbols);
		symb.Name = xstrdup(Name);
		symb.section = section;
		symb.weakSym = weakSym;
		symb.value = value; 
		return nSymbols-1; }
	
	// update existing symbol
	if( section == Type_Undefined )
		return symIndex;
	if( symbols[symIndex].section != Type_Undefined )
		return -1;
	symbols[symIndex].section = section;
	symbols[symIndex].weakSym = weakSym;
	symbols[symIndex].value = value;
	return symIndex;
}

/*
DWORD Symbol::getAddr(void) 
{
	DWORD symbBase = (int(section) < 0) ? 0 : 
		PeFILE::rvaToAddr(sections[section].baseRva);
	return symbBase + ;
}*/

DWORD Symbol::getRva(void)
{
	if(( section == Type_Absolute)
	||( section == Type_Undefined ))
		fatal_error("undefined symbol: %s\n", getName());
	return (isNeg(section) ? 0 : 
		sections[section].baseRva) + value;
}

u64 Symbol::getAddr(void)
{
	return (section == Type_Absolute) ?
	value : PeFILE::rvaToAddr64(getRva());
}

int symbolRva(int symbol) 
{
	assert(symbol >= 0);
	return symbols[symbol].getRva();
}

char* symbcat(char* symb, const char* str)
{
	int len = strlen(symb);
	char* result = xmalloc(strlen(str)+len+1);
	char* end = strrchr(symb, '@');
	if(end == NULL) end = symb+len;
	sprintf(result, "%.*s%s%s",  end-symb, symb, str, end);
	return result;
}

void fixSection(Section* sect, DWORD rva)
{
	sect->baseRva = rva;
	Void basePtr = PeFILE::rvaToPtr(rva);
	memcpy(basePtr, sect->rawData, sect->length);
	free(sect->rawData); sect->rawData = basePtr;
}

}
