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
	
SHITCALL cch* def_keepSymbol(char* name);
SHITCALL cch* def_freeBlock(u64 start, u64 end, int offset);
SHITCALL cch* def_symbol(u64 value, char* name, bool relocate);
SHITCALL cch* def_callPatch(u64 addr, SymbArg& s, bool hookMode);
SHITCALL cch* def_memNop(u64 start, u64 end);
SHITCALL cch* def_patchPtr(u64 addr, SymbArg2& s,
	char* hookSymb, bool ptrSize);

#endif
