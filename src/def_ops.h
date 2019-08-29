#ifndef _DEF_OPS_H_
#define _DEF_OPS_H_

struct SymbArg {
	u64 offset; union { char* name; 
		Linker::Symbol* symb; };
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
SHITCALL cch* def_freeBlock(u32 start, u32 end, int offset);
SHITCALL cch* def_symbol(u32 rva, char* name);
SHITCALL cch* def_const(u32 value, char* name);
SHITCALL cch* def_callPatch(u32 rva, SymbArg& s, bool hookMode);

SHITCALL cch* def_memKeep(u32 start, u32 end);
SHITCALL cch* def_memNop(u32 start, u32 end);
SHITCALL cch* def_memTrap(u32 start, u32 end);
SHITCALL cch* def_patchPtr(u32 rva, SymbArg2& s,
	char* hookSymb, int ptrSize);
SHITCALL cch* def_codeSkip(u32 start, u32 end);
SHITCALL cch* def_codeSkip(u32 start);
	
SHITCALL cch* def_fixSect(u32 start, u32 end, char* name);
SHITCALL cch* def_asmSect(char* name, char* str, u32 start);
SHITCALL cch* def_asmSect2(u32 rva, char* str, bool call);
SHITCALL cch* def_asmPatch(u32 start, u32 end, char* ins);
	
SHITCALL cch* def_funcRepl(u32 start, u32 end, char* name);
SHITCALL cch* def_import(char* name, char* symb);
SHITCALL cch* def_export(char* name, SymStrArg* forward);
SHITCALL cch* def_export(char* name, int iOrd, SymStrArg*);

SHITCALL cch* def_sectCreate(char* Name, int align);
SHITCALL cch* def_sectInsert(char* Name, u32 start, u32 end, DWORD ofs);
SHITCALL cch* def_sectAppend(char* Name, u32 start, u32 end, DWORD ofs);
cch* def_sectRevIns(char* Name, u32 start, u32 mid, u32 end);

SHITCALL cch* def_funcHook(u32 rva, int prologSz, char* name);
SHITCALL cch* def_prologMove(u32 rva, int prologSz, char* name);
SHITCALL cch* def_codeHook(u32 rva, int prologSz, char* str);
SHITCALL cch* def_makeJump(u32 rva, char* name, bool call);

SHITCALL cch* def_funcKill(u32 start, u32 end, u64* val);

SHITCALL cch* def_makeRet(DWORD& rva, u32 sz, u64* val);

SHITCALL cch* def_memPatch(u32 rva, cch* strPos);

#endif
