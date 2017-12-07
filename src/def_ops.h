#ifndef _DEF_OPS_H_
#define _DEF_OPS_H_

struct SymbArg {
	u64 offset; union { 
	char* name; DWORD symb; };
	u64 symInit(void);
	cch* parse(char* str);
	cch* parseNum(char* str);
};

struct SymbArg2 : SymbArg {
	bool offsetMode;
	cch* parse(char* str); };
	
struct SymStrArg : SymbArg { 
	char* str; cch* parse(char* str); };

REGCALL(1) cch* strArg_parse(char*& out, char* str);
	

SHITCALL cch* def_keepSymbol(char* name);
SHITCALL cch* def_freeBlock(u64 start, u64 end, int offset);
SHITCALL cch* def_symbol(u64 value, char* name);
SHITCALL cch* def_const(u64 value, char* name);
SHITCALL cch* def_callPatch(u64 addr, SymbArg& s, bool hookMode);
SHITCALL cch* def_memNop(u64 start, u64 end);
SHITCALL cch* def_patchPtr(u64 addr, SymbArg2& s,
	char* hookSymb, int ptrSize);
SHITCALL cch* def_asmPatch(u64 start, u64 end, char* ins);
	
SHITCALL cch* def_funcRepl(u64 start, u64 end, SymbArg& s);
SHITCALL cch* def_import(char* name, char* symb);
SHITCALL cch* def_export(char* name, SymStrArg* forward);

#endif
