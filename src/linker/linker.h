#ifndef _LINKER_H_
#define _LINKER_H_

namespace Linker {

// forward decl
struct Symbol;

// reloc interace
enum {
	Type_DIR32 = 0,
	Type_REL32 = 1, 
	Type_DIR32NB = 2,
	Type_DIR64 = 3 };
	
struct Reloc {
	WORD type;
	DWORD offset;
	Symbol* symbol;
	void fixup(struct Section* sect);
	inline Symbol* getSymb(void) {
		return symbol; }	
	inline Section* getSect();
	
};
extern Reloc* relocs;
extern DWORD nRelocs;
void addReloc(WORD type, DWORD offset, Symbol* symbol);
void relocs_fixup(void);

// section interface	
enum : short { 
	Type_NoLink = SHRT_MIN,
	Type_Keep = 0x4000,
	Type_RRef = 0x2000 };

struct Section {
	Section* next;

	const char* fileName;
	char* name; Void rawData;
	Reloc* relocs; DWORD nReloc;
	union { WORD type; struct { BYTE peType, 
		:5, rref:1, keep:1, noLink:1; }; };
	
	WORD align;
	DWORD baseRva, length;
	Symbol* symbol;
	
	
	void addReloc(WORD type, DWORD 
		offset, Symbol* symbol);
	
	
	Void endPtr() { return rawData+length; }
	
	
	bool isAlive() { return type != 0xFFFF; }
	bool isLinked() { return !isNeg(type); }
	bool isExec() { return isLinked() && (type & 
		PeSecTyp::Exec) && (type & PeSecTyp::Intd); }
	xarray<Reloc> rlcs() { return {relocs, nReloc}; }
	
	
	bool nameIs(cch* nn) {
		return name && !strcmp(name, nn); }
	
	inline bool isReal();	
		
		
};

Section* addSection(const char* fileName, const char* Name,
	void* rawData, WORD type, WORD align, DWORD length);
void destroy_section(Section& sect);
Section* findSection(const char* name);
Section* findSection2(const char* name);
void fixSection(Section* sect, DWORD rva);
int sectTypeFromName(cch* name);
bool sectTypeNormal(cch* Name);
cch* sectGrow(Section* sect, 
	DWORD offset, DWORD length);
cch* sectName(Section* sectId);
cch* sectFile(Section* sectId);
extern Section* sectRoot;

// symbol interface
static const auto Type_Undefined = (Section*)0;
static const auto Type_Relocate = (Section*)1;
static const auto Type_Absolute = (Section*)2;

struct Symbol {
	Symbol* next;
	Symbol* sectLst;

	char* Name;
	Section* section;
	Symbol* weakSym;
	DWORD value; 
	u64 getAddr(void); 
	
	DWORD getRva(void);
	
	
	cch* getName() { return Name ?
		Name : "##NO NAME##"; }
	
	bool nmcmp(cch* str) { return 
		Name && !strcmp(Name, str); }
		
	Section* getSect() { 
		if(!this || !section->isReal()) return 0;
		return notNull(section); }
		
};


enum {
	SYMBFLAG_STATIC = 1,
	SYMBFLAG_SECTION = 2
};

Symbol* findSymbol(cstr Name);
Symbol* findSymbol(const char* name); int symbolRva(Symbol* symbol);
Symbol* addSymbol(const char* name, Section* section, Symbol* weakSym, DWORD value, DWORD flags = 0);
Symbol* addSymbol2(const char* name, Section* section, Symbol* weakSym, DWORD value);
Symbol* addImport(const char* Name, const char* dllName, const char* importName);
char* symbcat(cch* symb, cch* str);
enum { SYMB_HASH_SIZE = 2048 };
extern Symbol* symbRoot[SYMB_HASH_SIZE];

// object functions
void library_load(const char* fileName,
	Void fileData, DWORD fileSize, bool whole);
void object_load(const char* fileName, 
	Void objData, DWORD objSize);
void imports_parse(void);
void imports_resolve(void);
const char* nullchk(const char* str, const char* limit);
void gc_sections(void);
void gc_sections2(void);

// exports interface
void addExport(char* name, uint ord, 
	cch* frwd, DWORD offset, DWORD oldRva);
void exports_symbfix();
void exports_resolve();
void exports_addSymb(Symbol* symb, char* importName);
Symbol* exports_getExpSym(
	char*& dllNane, char*& importName);
int needSymbol(cch* Name);

extern xarray<char*> keep_list;
static inline
void keepSymbol(char* name) {
	keep_list.push_back(name); }
	
#define LINKER_ENUM_SECTIONS(sect, ...) \
	RingList_enum(Linker::sectRoot, sect, __VA_ARGS__)
#define LINKER_ENUM_SYMBOLS(symb, ...) \
	for(Symbol* symb : symbRoot) \
		for(;symb;symb=symb->next) { __VA_ARGS__ }
	
Section* Reloc::getSect() {
	return symbol->getSect(); }
	
bool Section::isReal() {
	return this > Type_Absolute; }
	
	
// section merging
void mergeSect_init(Section* sect);
void mergeSect_step1(void);
void mergeSect_step2(void);

void rsrc_build(void);


void link_step1(void);
void link_step2(void);
void symTab_build(void);


Symbol* symb_find(cch* name, strcmp_t cmp);

}
#endif
