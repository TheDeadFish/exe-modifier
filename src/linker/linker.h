#ifndef _LINKER_H_
#define _LINKER_H_

namespace Linker {

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


	};
extern Reloc* relocs;
extern DWORD nRelocs;
void addReloc(WORD type, DWORD offset, DWORD symbol);
void relocs_fixup(void);

// section interface	
struct Section {
	const char* fileName;
	char* name; Void rawData;
	Reloc* relocs; DWORD nReloc;
	WORD type; WORD align;
	DWORD baseRva, length;
	
	void addReloc(WORD type, DWORD 
		offset, DWORD symbol);
	
	
	Void endPtr() { return rawData+length; }
	
	
	bool isExec() { return is_one_of(type, 3, 4); }
};
extern Section* sections;
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

// symbol interface
enum : DWORD {
	Type_Undefined = DWORD(-1),
	Type_Relocate = DWORD(-2),
	Type_Absolute = DWORD(-3),
	Type_Import	= DWORD(-4) };
struct Symbol {
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
};
extern Symbol* symbols;
extern DWORD nSymbols;
int findSymbol(const char* name); int symbolRva(int symbol);
int addSymbol(const char* name, DWORD section, DWORD weakSym, DWORD value);
int addImport(const char* Name, const char* dllName, const char* importName);
static inline bool isUndefSymb(int symb) { return (symb < 0)
	|| (symbols[symb].section == Type_Undefined); }
char* symbcat(cch* symb, cch* str);

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

}
#endif
