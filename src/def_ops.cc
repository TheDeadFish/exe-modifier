#include "stdafx.h"
#include "exe_mod.h"
#include "def_ops.h"

SHITCALL cch* defFileGetNumber(u64& out, char* str);
SHITCALL bool defFileIsAddress(char* str);
SHITCALL char* defFileGetNumPos(char* str);

u64 SymbArg::symInit()
{
	symb = Linker::addSymbol(name, 
	Linker::Type_Undefined, -1, 0);
	return offset;
}

cch* SymbArg::parseNum(char* str)
{
	return defFileGetNumber(offset, str);
}

cch* SymbArg::parse(char* str)
{
	if(defFileIsAddress(str)) { 
	name = 0; return parseNum(str); }
	name = str; offset = 0;
	if(char* numPos = defFileGetNumPos(str)) {
		cch* ret = parseNum(numPos); 
		*numPos = '\0'; return ret; }
}

cch* SymbArg2::parse(char* str)
{
	if(offsetMode = (*str == '@')) str++;
	return SymbArg::parse(str);
}

struct callPatchCore_t {
	bool relative;
	byte addrType;
	WORD reserved;
	int patchOffset;
};

callPatchCore_t callPatchCore(
	Void ptr, bool create)
{
	callPatchCore_t ret = { 
		true, 1, 0, 2};
	
	// 0x25FF: jmp indirect
	if( ptr.word() == 0x25FF ) 
	{ 
		ptr.word() = 0xE990;
	}	

	// 0x15FF: call indirect
	ei( ptr.word() == 0x15FF ) 
	{
		ptr.word() = 0xE890;
	}
	
	// 0xA1: move eax, [address]
	ei((ptr[0] == 0xA1))
	{
		ptr[0] = 0xB8;
		ret.patchOffset = 1;
		ret.relative = false;
	}
	
	// 0x??8B: move ea?, [address]
	ei((ptr.word() & 0xC7FF) == 0x58B)
	{
		int r = (ptr.word() >> 11);
		ptr.word() = 0xB890 + (r << 8);
		ret.relative = false;
	}
	
	// 0x8X0F: conditional
	ei((ptr.word() & 0xF0FF) == 0x800F)
	{
		ret.addrType = 2;
	}
	
	// 0xE8: call relative
	// 0xE9: jump relative	
	ei((ptr[0] == 0xE8 )
	||(ptr[0] == 0xE9 )) {
		ret.patchOffset = 1;
		ret.addrType = 2;
	}
	
	// anything else
	else {
		ret.patchOffset = 1;
		ret.addrType = 0;
		if(create == true)
			ptr[0] = 0xE9;
	}
	
	return ret;
}


cch* def_keepSymbol(char* name)
{
	Linker::keepSymbol(name);
	return 0;
}

cch* def_freeBlock(u64 start,
	u64 end, int offset)
{
	DWORD length = end-start;
	Void ptr = PeFILE::patchChk(start, length);
	if(ptr == NULL) return "bad address range";
	PeFILE::clearSpace(PeFILE::addrToRva(
		start), length, offset); return 0;
}

cch* def_symbol(u64 value, 
	char* name, bool relocate)
{
	Linker::addSymbol(name, relocate ? Linker::Type_Relocate
		: Linker::Type_Absolute, -1, value); return 0;
}

cch* def_callPatch(u64 addr, SymbArg& s, bool hookMode)
{
	printf("%I64X, %s, %I64X\n", addr, s.name, s.offset);


	Void ptr = PeFILE::patchChk(addr, 5);
	if(ptr == NULL) return "bad patch address";
	auto cp = callPatchCore(ptr, true);
	
	// implement hook mode
	Void patchPtr = ptr+cp.patchOffset;
	if(hookMode == true) {
		if(!s.name) return "CALLHOOK must be symbol";
		if(cp.addrType != 2) return "CALLHOOK must be relative";
		DWORD oldCall = patchPtr.dword()+addr+cp.patchOffset+4;
		Linker::addSymbol(s.name, Linker::Type_Relocate, -1, oldCall);
		s.name = Linker::symbcat(s.name, "_hook");
	} SCOPE_EXIT(if(hookMode == true) free(s.name););
	
	// apply patch
	int rva = PeFILE::addrToRva(addr);
	PeFILE::Relocs_Remove(rva, cp.patchOffset+4);
	if(s.name) { patchPtr.dword() = s.symInit();
		Linker::addReloc(cp.relative ? Linker::Type_REL32 : 
			Linker::Type_DIR32,	rva+cp.patchOffset, s.symb);		
	} else {
		int patchAddr = addr+cp.patchOffset;
		if( cp.relative == true ) {
			patchPtr.dword() = s.offset-(patchAddr+4);
		} else {
			patchPtr.dword() = PeFILE::addrToRva(s.offset);
			PeFILE::Relocs_Add(rva+cp.patchOffset);
		}
	}
	
	return 0;
}

cch* def_memNop(u64 start, u64 end)
{
	DWORD length = end-start;
	Void ptr = PeFILE::patchChk(start, length);
	if(ptr == NULL) return "bad address range";
	PeFILE::Relocs_Remove(PeFILE::addrToRva(start), length);
	memset(ptr, 0x90, length); return 0;
}

cch* def_patchPtr(u64 addr, SymbArg2& s,
	char* hookSymb, bool ptrSize)
{

#if 0
	ptrSize &= PeFILE::peFile.PE64;
	Void ptr = PeFILE::patchChk(
		addr, ptrSize ? 8 : 4);
	if(!ptr) return "bad patch address";
	
	// handle hook mode
	int rva = PeFILE::addrToRva(addr);
	if(hookSymb) {
		DWORD hookAddr = ptr.dword();
		bool relocate = PeFILE::Reloc_Find(rva);
		Linker::addSymbol(hookSymb, relocate ? Linker::Type_Relocate
		: Linker::Type_Absolute, -1, hookAddr); }
		
	// apply patch
	if(!s.offsetMode) ptr.dword() = 0;
	if(s.name) { 
		
	
	
		
		ptr.dword() += s.symInit();
		Linker::addReloc(ptrSize ? 
			Linker::Type_DIR64 : Linker::Type_DIR32,
			PeFILE::addrToRva(addr), symbol);
			
	} else {
		
	
	
	
	}
	
	
	

	/*
		// process arguments (1&2)	
	bool offsetMode = false;
	if(arg2[0] == '@') {
		offsetMode = true;
		arg2 += 1; }
	DWORD addr = getNumber(arg1);
	Void ptr = PeFILE::patchChk(addr, 4);
	if(ptr == NULL)
		defBad(, arg1);
		
	// handle hook mode (arg3)
	
	if(hookMode == true) {
		

	// apply patch
	DWORD symbol = getSymbol(arg2, ptr);
	if(symbol == DWORD(-1)) {
		DWORD value = getNumber(arg2);
		if(offsetMode == true)
			value += ptr.dword();
		ptr.dword() = value;
	} else {
		if(offsetMode == true)
			defBad("@mode invalid", arg2);
		PeFILE::Relocs_Remove(rva);
		Linker::addReloc(Linker::Type_DIR32, 
			PeFILE::addrToRva(addr), symbol);
	}















*/

#endif



}

