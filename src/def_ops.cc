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

cch* SymStrArg::parse(char* str)
{
	if(*str == '"') { 
	str++; char* end = strrchr(str, '"');
	if(!end) return "unterminated string"; 
	*end = '\0'; ZINIT; this->str = str; 
	return 0; } return SymbArg::parse(str);
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

cch* def_symbol(u64 value, char* name)
{
	printf("symb: %s, %I64X\n", name, value);

	Linker::addSymbol(name, Linker::Type_Relocate,
		-1, PeFILE::addrToRva64(value)); return 0;
}

cch* def_const(u64 value, char* name)
{
	Linker::addSymbol(name, Linker::Type_Absolute,
		-1, value); return 0;
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
	char* hookSymb, int size)
{
	if(size & 2) size += PeFILE::peFile.PE64;
	Void ptr = PeFILE::patchChk(addr, (size&1) ? 8 : 4);
	if(!ptr) return "bad patch address";
	
	// handle hook mode
	int rva = PeFILE::addrToRva(addr);
	if(hookSymb) { u64 addr = (size&1) ?
		ptr.ref<u64>() : ptr.dword();
		if(size & 2){ def_symbol(addr, hookSymb);
		} else { def_const(addr, hookSymb); }	
	}
		
	// apply patch
	if(!s.offsetMode) if(size&1) ptr.ref
		<u64>() = 0; else ptr.dword() = 0;
	if(size&1) ptr.ref<u64>() += s.offset;
	else ptr.dword() += s.offset;
	
	// register relocation
	if(s.name) { s.symInit();
		PeFILE::Relocs_Remove(rva);
		Linker::addReloc((size&1) ? Linker::Type_DIR64
			: Linker::Type_DIR32, rva, s.symb);	
	}
}

SHITCALL cch* def_funcRepl(u64 start, u64 end, SymbArg& s)
{
	IFRET(def_freeBlock(start, end, 5));
	return def_callPatch(start, s, 0);
}

SHITCALL cch* def_import(char* name, char* symb)
{
	char* dllName = strtok(name, ";");
	char* impName = strtok(NULL, ";");	
	if(!impName) return "IMPORTDEF import name bad";
	
	if(Linker::addImport(symb, dllName, impName) < 0)
		return "IMPORTDEF symbol allready defined";
	return 0;
}

SHITCALL cch* def_export(char* str, SymStrArg* frwd)
{
	// get export and ordinal name
	char* name = strtok(str, ";: "); 
	char* ord = strtok(NULL, ";: ");
	if(is_one_of(*name, '#', '@')) 
		ord = release(name)+1;

	// get ordinal number
	u64 iOrd = 0; if(ord) { 
		if(is_one_of(*ord, '#', '@')) ord++;
		IFRET(defFileGetNumber(iOrd, ord)); }

	// lookup export
	auto exp = PeFILE::peExp.find(name, iOrd); if(exp.err) {
		if(exp.err > 0) return "EXPORTDEF: name ordinal missmatch";
		return "EXPORTDEF: mistake catcher; existing export specified "
			"by ordinal when name exists"; }
	if(!exp) { exp.slot = &PeFILE::peExp.add(name, iOrd); }
	
	// handle immidiate, forwarder
	if(frwd){ if(frwd->str) { 
		PeFILE::peExp.setFrwd(*exp,frwd->str); return 0; }
		if(!frwd->name) { PeFILE::peExp.setRva(*exp,
		PeFILE::addrToRva64(frwd->offset)); return 0; }}
		
	// handle symbol
	DWORD symbol = Linker::addSymbol(frwd ? frwd->name
		: name, Linker::Type_Undefined, -1, 0);
	DWORD symbOffset = frwd ? frwd->offset : 0;
	if(exp->frwd) PeFILE::peExp.setFrwd(*exp, 0);
	Linker::addExport(name, iOrd, 
		symbol, symbOffset, exp->rva);
	return 0;
}

