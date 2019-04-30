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

Section** sections;
DWORD nSections;
Symbol* symbols = xMalloc(65536);
DWORD nSymbols;
Reloc* relocs;
DWORD nRelocs;
xarray<char*> keep_list;
Section* sectRoot;
Symbol* symbRoot;

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
	auto& sect = *(Section*)xMalloc(1);
	xNextAlloc(sections, nSections) = &sect;
	ringList_add(sectRoot, &sect);

	sect.fileName = fileName;
	sect.name = xstrdup(Name);
	sect.rawData = xcalloc(length);
	if(rawData != NULL)
		memcpy(sect.rawData, rawData, length);
	sect.relocs = 0; sect.nReloc = 0;
	sect.type = type; sect.align = align;
	sect.baseRva = baseRva; sect.length = length; 
	
	// create symbol
	if(Name && *Name) { addSymbol(
		Name, nSections-1, 0, 0); }
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
	LINKER_ENUM_SECTIONS(sect, 
		if(sect->name && !strcmp(sect->name, name))
			return sect; ); return NULL;
}

Symbol* findSymbol(const char* Name)
{
	if(Name != NULL)
	LINKER_ENUM_SYMBOLS(symb,
		if((symb->Name != NULL)
		&&(!strcmp(symb->Name, Name)))
		  return symb; );
	return NULL;
}

Symbol* addSymbol(const char* Name, DWORD section, Symbol* weakSym, DWORD value)
{
	auto* symb = findSymbol(Name);
	if(!symb) {
	
		// create new symbol
		symb = &symbols[nSymbols++];
		ringList_add(symbRoot, symb);
		symb->Name = xstrdup(Name);
		
	} else {
	
		// update existing symbol
		if( section == Type_Undefined ) 
			return symb;
		if( symb->section != Type_Undefined ) 
			return NULL;
	}

	symb->section = section;
	symb->weakSym = weakSym;
	symb->value = value; 
	return symb;
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
		sections[section]->baseRva) + value;
}

u64 Symbol::getAddr(void)
{
	return (section == Type_Absolute) ?
	value : PeFILE::rvaToAddr64(getRva());
}

int symbolRva(Symbol* symbol) 
{
	return symbol ? symbol->getRva() : 0;
}

char* symbcat(cch* symb, cch* str)
{
	cstr symb1 = symb;
	cstr symb2 = cstr_split(symb1, '@');
	return xstrfmt("%v%s%s", symb2, str, 
		symb1.data ? symb1.data-1 : "");
}

void fixSection(Section* sect, DWORD rva)
{
	sect->baseRva = rva;
	Void basePtr = PeFILE::rvaToPtr(rva);
	memcpy(basePtr, sect->rawData, sect->length);
	free(sect->rawData); sect->rawData = basePtr;
}

int sectTypeFromName(cch* Name)
{
	static const std::pair<cch*,int> typeList[] = {
		{".data", PeSecTyp::Data}, {".bss", PeSecTyp::Bss},
		{".rdata", PeSecTyp::RData}, {".text", PeSecTyp::Text},
		{".idata", SHRT_MIN}, {"@patch", SHRT_MIN}};
	
	cch* end = Name+1;
	for(;!is_one_of(*end, '$', '.', 0); end++);
	cstr str((char*)Name, (char*)end);
	
	for(auto& type : typeList) {
		if(!str.cmp(type.first)) 
			return type.second; }
	return 0;
}

cch* sectGrow(Section* sect, 
	DWORD offset, DWORD length)
{
	// resize the section
	if(sect->baseRva) return "section fixed";
	{ Void ofsPos = xrealloc(sect->rawData, 
		sect->length+length)+offset;
	memmove(ofsPos+length, ofsPos, sect->length-offset);
	sect->length += length; }
	
	// alter symbols
	Symbol* iSectSymb = 0;
	LINKER_ENUM_SYMBOLS(symb, 
		if(sectPtr(symb->section) == sect) {
		if(symb->nmcmp(sect->name)) { iSectSymb = symb; }
		ei(symb->value >= offset) { symb->value += length; }}
	);

	// alter relocs
	for(auto& reloc : RngRBL(
	sect->relocs, sect->nReloc)) {
	if(reloc.offset >= offset) {
		if(reloc.symbol == iSectSymb)
			return "sect symbol referenced";
		reloc.offset += length; 
	}}
	
	return NULL;
}

cch* sectName(DWORD sectId)
{
	switch(sectId) {
	case Type_Undefined: return "[undefined]";
	case Type_Relocate: return "[relocate]";
	case Type_Absolute: return "[absolute]";
	case Type_Import: return "[import]"; }
	cch* name = sections[sectId]->name;
	return name ? name : "[no name]";
}

cch* sectFile(DWORD sectId)
{
	if(sectId >= nSections) return "[no file]";
	cch* name = sections[sectId]->fileName;
	return name ? name : "[no file]";
}

}
