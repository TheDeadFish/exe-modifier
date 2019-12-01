#include "stdafx.h"
#include "../exe_mod.h"
#include "linker.h"
#include <conio.h>
DebugTimer dbgTimer;

namespace Linker {
#include "library.cpp"
#include "object.cpp"
#include "relocs.cpp"
#include "imports.cpp"
#include "exports.cpp"
#include "gc-sections.cpp"
#include "symtab.cpp"

DWORD symb_hash(cch* str) 
{
	while(*str == '_') str++;
	DWORD hash = 5381;
	while(1) { u8 ch = *str; str++;
		if(!ch || (ch == '@')) break;
		hash = (hash << 5) + hash + ch; }
	return hash;
}	

void symb_insert(Symbol* symb)
{
	if(!symb->Name) return;
	DWORD hash = symb_hash(symb->Name);
	Symbol*& slot = symbRoot[hash % SYMB_HASH_SIZE];
	if(slot) { symb->next = slot; }
	slot = symb; 
}

static
Symbol* symb_find(cch* name)
{
	if(name) {
	DWORD hash = symb_hash(name);
	Symbol* symb = symbRoot[hash % SYMB_HASH_SIZE];
	for(; symb; symb = symb->next) {
		if(!strcmp(symb->Name, name))
			return symb; }
	}  return NULL;
}

Reloc* relocs;
DWORD nRelocs;
xarray<char*> keep_list;
Section* sectRoot;
Symbol* symbRoot[SYMB_HASH_SIZE];

const char* nullchk(const char* str, const char* limit)
{
	while(str < limit)
	  if(*str++ == '\0')
		return str;
	return NULL;
}

Section* addSection(const char* fileName, const char* Name,
	void* rawData, WORD type, WORD align, DWORD length)
{
	auto& sect = *(Section*)xCalloc(1);
	ringList_add(sectRoot, &sect);

	sect.fileName = fileName;
	sect.name = xstrdup(Name);
	sect.rawData = xcalloc(length);
	if(rawData != NULL)
		memcpy(sect.rawData, rawData, length);
	sect.type = type; sect.align = max(align,1);
	sect.length = length;
	
	return &sect;
};

void destroy_section(Section& section)
{
	free_ref(section.relocs);
	free_ref(section.rawData);
	free_ref(section.name);
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

Section* findSection2(const char* name)
{
	IFRET(findSection(name));
	Symbol* symb = findSymbol(name);
	if(!symb || !symb->section->isReal()
	|| symb->value) return NULL;
	return symb->section;
}




Symbol* findSymbol(const char* Name)
{
	return symb_find(Name);
}

Symbol* findSymbol(cstr Name)
{
	if(!Name) return NULL;
	NullTermTmp tmp(Name);
	return findSymbol(tmp);
}

Symbol* addSymbol(const char* Name, Section* section, Symbol* weakSym, DWORD value)
{
	auto* symb = findSymbol(Name);
	if(!symb) {
	
		// create new symbol
		symb = xCalloc(1);
		symb->Name = xstrdup(Name);
		symb_insert(symb);
		
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
	return (!section->isReal() ? 0 : 
		section->baseRva) + value;
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

char* symbfix(cch* symb)
{
	if(!symb || ((*symb != '_') && !isalpha(*symb))) return xstrdup(symb);
	if(!x64Mode()) return xstrfmt("_%s", symb);
	char* ret = xstrdup(symb); char* at = strrchr(ret, '@'); 
	if(at) *at = '\0'; return ret;
}

Symbol* addSymbol2(const char* Name, Section* section, Symbol* weakSym, DWORD value)
{
	if(g_noSymFix == false)
		return addSymbol(xstr(symbfix(Name)), section, weakSym, value);
	return addSymbol(Name, section, weakSym, value);
}

void fixSection(Section* sect, DWORD rva)
{
	sect->baseRva = rva;
	if(sect->rawData) {
		Void basePtr = PeFILE::rvaToPtr(rva);
		memcpy(basePtr, sect->rawData, sect->length);
		free(sect->rawData); sect->rawData = basePtr;
	}
}

int sectTypeFromName(cch* Name)
{
	static const std::pair<cch*,int> typeList[] = {
		{".data", PeSecTyp::Data}, {".bss", PeSecTyp::Bss},
		{".rdata", PeSecTyp::RData}, {".text", PeSecTyp::Text},
		{".idata", Type_NoLink}, {"@patch", Type_NoLink}, 
		{".rsrc", Type_NoLink|Type_Keep },
		{".pdata", Type_NoLink|Type_RRef },
		{".xdata", Type_NoLink }};
	
	cch* end = Name+1;
	for(;!is_one_of(*end, '$', '.', 0); end++);
	cstr str((char*)Name, (char*)end);
	
	for(auto& type : typeList) {
		if(!str.cmp(type.first)) 
			return type.second; }
	return 0;
}

bool sectTypeNormal(cch* Name)
{
	if(strScmp(Name, ".idata")) return true;
	static const cch* names[] = {
		".data", ".bss", ".rdata", ".text", ".rsrc"};
	for(cch* x : names) {
		if(!strcmp(Name, x)) return true; }
	return false;
}

#if 0
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
		if(symb->section == sect) {
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
#endif


cch* sectName(Section* sectId)
{
	if(sectId == Type_Undefined)  return "[undefined]";
	if(sectId == Type_Relocate)  return "[relocate]";
	if(sectId == Type_Absolute)  return "[absolute]";
	cch* name = sectId->name;
	return name ? name : "[no name]";
}

cch* sectFile(Section* sectId)
{
	if(sectId <= Type_Absolute) return "[no file]";
	cch* name = sectId->fileName;
	return name ? name : "[no file]";
}

struct MergeList { 
	cch* name; Symbol* symbol; };
xarray<MergeList> mergeList;

static
Symbol* mergeSect_symb(Section* sect,
	 cch* prefix, cch* sect_name)
{
	xstr name = symbcat(prefix, sect_name);
	Symbol* symb = addSymbol(name, sect, 0, 0);
	if(!symb) fatal_error("duplicate symbol: %s", name.data);
	return symb;
}

void mergeSect_init(Section* sect)
{
	Section* ms;
	Symbol* stop;
	
	// find the section
	for(auto& x : mergeList) {
		if(sect->nameIs(x.name)) { stop = x.symbol; 
			ms = stop->section; goto FOUND_SECT; }}
		
	// create the section
	ms = addSection(NULL, NULL, 0, 0, 0, 0);
	mergeSect_symb(ms, archStr->sectionStart, sect->name);
	stop = mergeSect_symb(ms, archStr->sectionStop, sect->name);
	mergeList.push_back(sect->name, stop);
	
FOUND_SECT:
	max_ref(ms->align, sect->align);
	ms->type |= sect->type;
	DWORD offset = ALIGN(ms->length, sect->align-1);
	stop->value = ms->length = offset + sect->length;
	ms->addReloc(-1, offset, sect->symbol);
}

void mergeSect_step1(void)
{
	for(auto& x : mergeList) {
		auto* sect = x.symbol->section;
		for(auto& reloc : sect->rlcs()) {
			reloc.symbol->section->baseRva = 1; }
	}
}

void mergeSect_step2(void)
{
	for(auto& x : mergeList) {
		auto* sect = x.symbol->section;
		for(auto& reloc : sect->rlcs()) {
			fixSection(reloc.symbol->section,
				sect->baseRva+reloc.offset); }
	}
}

void rsrc_build(void)
{
	LINKER_ENUM_SECTIONS(sect, 
		if(sect->nameIs(".rsrc")) {
			PeFILE::setRes(sect->rawData, sect->length);
		}
	)
}

void except_step1(void);
void except_step2(void);

void page_step1(void)
{
	// detect driver mode
	for(auto& sect : PeFILE::peFile.sects)
		if(sect.type() & PeSecTyp::NoPage)
			goto DRIVER_MODE;
	return;

DRIVER_MODE:
	LINKER_ENUM_SECTIONS(sect,
		sect->peType |= PeSecTyp::NoPage;
	)
}

void link_step1(void)
{
	exports_symbfix();
	gc_sections();
	imports_parse();
	mergeSect_step1();
	except_step1();
	page_step1();
}

void link_step2(void)
{
	mergeSect_step2();
	except_step2();
	imports_resolve();
	exports_resolve();
	relocs_fixup();
	rsrc_build();	
}

}