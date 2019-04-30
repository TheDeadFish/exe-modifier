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
	DWORD symbol;
	void fixup(struct Section* sect);
	inline Symbol* getSymb(void);
};
extern Reloc* relocs;
extern DWORD nRelocs;
void addReloc(WORD type, DWORD offset, DWORD symbol);
void relocs_fixup(void);

// section interface	
struct Section {
	Section* next;

	const char* fileName;
	char* name; Void rawData;
	Reloc* relocs; DWORD nReloc;
	union { WORD type; struct { BYTE 
		peType, :6, keep:1, noLink:1; }; };
	
	WORD align;
	DWORD baseRva, length;
	
	void addReloc(WORD type, DWORD 
		offset, DWORD symbol);
	
	
	Void endPtr() { return rawData+length; }
	
	
	bool isAlive() { return type != 0xFFFF; }
	bool isLinked() { return !isNeg(type); }
	bool isExec() { return isLinked() && (type & 
		PeSecTyp::Exec) && (type & PeSecTyp::Intd); }
	xarray<Reloc> rlcs() { return {relocs, nReloc}; }
};
extern Section** sections;
extern DWORD nSections;
DWORD addSection(const char* fileName, const char* Name,
	void* rawData, WORD type, WORD align, 
	DWORD baseRva, DWORD length);
void destroy_section(Section& sect);
Section* findSection(const char* name);
void fixSection(Section* sect, DWORD rva);
int sectTypeFromName(cch* name);
cch* sectGrow(Section* sect, 
	DWORD offset, DWORD length);
cch* sectName(DWORD sectId);
cch* sectFile(DWORD sectId);
extern Section* sectRoot;


static
Section* sectPtr(DWORD sectId) {
	return (sectId < nSections) ?
		sections[sectId] : 0; }

// symbol interface
enum : DWORD {
	Type_Undefined = DWORD(-1),
	Type_Relocate = DWORD(-2),
	Type_Absolute = DWORD(-3),
	Type_Import	= DWORD(-4) };
struct Symbol {
	Symbol* next;

	char* Name;
	DWORD section;
	DWORD weakSym;
	DWORD value; 
	u64 getAddr(void); 
	
	DWORD getRva(void);
	
	
	cch* getName() { return Name ?
		Name : "##NO NAME##"; }
	
	bool nmcmp(cch* str) { return 
		Name && !strcmp(Name, str); }
		
	Section* getSect() { 
		if(!this || isNeg(section)) return 0;
		return notNull(sections[section]); }
		
};
extern Symbol* symbols;
extern DWORD nSymbols;
int findSymbol(const char* name); int symbolRva(int symbol);
int addSymbol(const char* name, DWORD section, DWORD weakSym, DWORD value);
int addImport(const char* Name, const char* dllName, const char* importName);
static inline bool isUndefSymb(int symb) { return (symb < 0)
	|| (symbols[symb].section == Type_Undefined); }
char* symbcat(cch* symb, cch* str);
extern Symbol* symbRoot;

static inline
Symbol* getSymbol(DWORD symb) { return 
	isNeg(symb) ? 0 : notNull(symbols+symb); }
static inline
Symbol* findSymbol2(const char* name) {
	return getSymbol(findSymbol(name)); }

// object functions
void library_load(const char* fileName,
	Void fileData, DWORD fileSize);
void object_load(const char* fileName, 
	Void objData, DWORD objSize);
void imports_parse(void);
void imports_resolve(void);
const char* nullchk(const char* str, const char* limit);
void gc_sections(void);

// exports interface
void addExport(char* name, uint ord, 
	DWORD symbol, DWORD offset, DWORD oldRva);
void exports_symbfix();
void exports_resolve();
void exports_addSymb(int symb, char* importName);
int exports_getExpSym(
	char*& dllNane, char*& importName);

extern xarray<char*> keep_list;
static inline
void keepSymbol(char* name) {
	keep_list.push_back(name); }
	
#define LINKER_ENUM_SECTIONS(sect, ...) \
	RingList_enum(Linker::sectRoot, sect, __VA_ARGS__)
#define LINKER_ENUM_SYMBOLS(symb, ...) \
	RingList_enum(Linker::symbRoot, symb, __VA_ARGS__)
	
// 
Symbol* Reloc::getSymb(void) {
	return symbols+symbol; }

}
#endif
