#include "stdafx.h"
#include "../exe_mod.h"
#include "linker.h"
#include <conio.h>
namespace Linker {
#include "library.cpp"
#include "object.cpp"
#include "relocs.cpp"
#include "imports.cpp"
#include "gc-sections.cpp"

Section* sections;
DWORD nSections;
Symbol* symbols;
DWORD nSymbols;
Reloc* relocs;
DWORD nRelocs;

const char* nullchk(const char* str, const char* limit)
{
	while(str < limit)
	  if(*str++ == '\0')
		return str;
	return NULL;
}

DWORD addSection(const char* fileName, const char* Name,
	void* rawData, DWORD iReloc, DWORD nReloc, 
	WORD type, WORD align, DWORD baseRva, DWORD length)
{
	auto& sect = xNextAlloc(sections, nSections);
	sect.fileName = fileName;
	sect.name = xstrdup(Name);
	sect.rawData = xmalloc(length);
	memset(sect.rawData, 0, length);
	if(rawData != NULL)
		memcpy(sect.rawData, rawData, length);
	sect.iReloc = iReloc; sect.nReloc = nReloc;
	sect.type = type; sect.align = align;
	sect.baseRva = baseRva; sect.length = length; 
	return nSections-1;
};

void destroy_section(Section& section)
{
	for(int i = 0; i < section.nReloc; i++)
		relocs[section.iReloc+i].type = -1;
	section.name[0] = '\0';
	free_ref(section.rawData);
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

DWORD Symbol::getAddr(void) 
{
	DWORD symbBase = (int(section) < 0) ? 0 : 
		PeFILE::rvaToAddr(sections[section].baseRva);
	return symbBase + this->value;
}

int symbolRva(int symbol) 
{
	assert(symbol >= 0);
	auto& symb = symbols[symbol];
	if(( symb.section == Type_Absolute)
	||( symb.section == Type_Undefined ))
		fatal_error("undefined symbol: %s\n",
			symb.Name ? symb.Name :"##NO NAME##" );
	return PeFILE::addrToRva(symb.getAddr());
}
}
