#include "stdafx.h"
#include "exe_mod.h"
#include "def_ops.h"

SHITCALL cch* defFileGetNumber(u64& out, char* str);
SHITCALL bool defFileIsAddress(char* str);
SHITCALL char* defFileGetNumPos(char* str);

// reg/mem helpers
SHITCALL int regMemLen(Void ptr) { 
	int len = 1;
	u8 mod = ptr[0]>>6, base = ptr[0]&7;
	if((mod != 3)&&(base == 4)) {
		base = ptr[1]&7; len++; }
	if((!mod && (base == 5))||(mod == 2))
	len+=4; if(mod == 1) len++; return len; }

// 
int getRel32Rva(Void ptr, int extra = 0) {
	return PeFILE::ptrToRva(ptr)+ptr.dword()+extra+4; } 
int getDir32Rva(Void ptr) {
	return PeFILE::addrToRva(ptr.dword()); }

	
	
	

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

cch* strArg_parse(char*& out, char* str)
{
	char *beg = 0; if(*str == '"') { str++; 
	char* end = strrchr(str, '"'); if(!end) 
	return "unterminated string"; *end = 0;
	beg = str; } out = beg; return 0;
}

cch* SymStrArg::parse(char* in)
{
	ZINIT;
	cch* err = strArg_parse(str, in);
	if(err || str) return err;
	return SymbArg::parse(in);
}

struct callPatchCore_t {
	byte oldType;     // type 0: relative
	byte newType;     // type 1: absolute
	int patchOffset;  // type 2: indirect
};

static bool x64Mode() { return PeFILE::peFile.PE64; }
static int ptrSize() { return x64Mode() ? 64 : 32; }

callPatchCore_t callPatchCore(
	Void ptr, bool create)
{

	if(x64Mode()) {
	
		// lea ra?, [rip+disp32]
		if((ptr.dword() & 0xC7FFF0) == 0x058D40) 
		{ return {1,1,3}; }
	
	} else {
	
		// 0xC7: mov r/m, imm32
		if(ptr[0] == 0xC7) {
			return {1, 1, regMemLen(ptr+1)+1}; }
			
		// 0xB8+r: mov ea?, imm32
		if(inRng(ptr[0], 0xB8, 0xBF)) { 
			return {1, 1, 1}; }
			
		// 0xA1: move eax, [address]
		if(ptr[0] == 0xA1) { 
			ptr[0] = 0xB8; return {2, 1, 1}; }
			
			
		// 0x??8B: move ea?, [address]
		if((ptr.word() & 0xC7FF) == 0x58B)
		{
			int r = (ptr.word() >> 11);
			ptr.word() = 0xB890 + (r << 8);
			return {2, 1, 2};
		}
	}
		
	// 0xE8: call relative
	// 0xE9: jump relative	
	if((ptr[0] == 0xE8 )
	||(ptr[0] == 0xE9 )) { 
		return {0,0,1}; }
		
	// 0x25FF: jmp indirect
	if( ptr.word() == 0x25FF ) { 
		ptr.word() = 0xE990;
		return {2, 0, 2}; }

	// 0x15FF: call indirect
	ei( ptr.word() == 0x15FF ) {
		ptr.word() = 0xE890;
		return {2, 0, 2}; }
	
	// 0x8X0F: conditional
	ei((ptr.word() & 0xF0FF) == 0x800F) {
		return {0,0, 2}; }
	
	return {0,0,-1};
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
	Void ptr = PeFILE::patchChk(addr, 5);
	if(ptr == NULL) return "bad patch address";
	auto cp = callPatchCore(ptr, true);
	if(cp.patchOffset < 0) return "unsuported instruction";
	
	// implement hook mode
	Void patchPtr = ptr+cp.patchOffset;
	if(hookMode == true) {
		if(!s.name) return "CALLHOOK must be symbol";
		if(cp.oldType == 2) return "CALLHOOK must not be indirect";
		
		int oldCall = cp.oldType 
			? getDir32Rva(ptr+cp.patchOffset)
			: getRel32Rva(ptr+cp.patchOffset);

		Linker::addSymbol(s.name, Linker::Type_Relocate, -1, oldCall);
		
		s.name = Linker::symbcat(s.name, "_hook");
	} SCOPE_EXIT(if(hookMode == true) free(s.name););
	
	// apply patch
	int rva = PeFILE::addrToRva(addr);
	PeFILE::Relocs_Remove(rva, cp.patchOffset+4);
	if(s.name) { patchPtr.dword() = s.symInit();
		Linker::addReloc(cp.newType ? Linker::Type_DIR32 : 
			Linker::Type_REL32,	rva+cp.patchOffset, s.symb);		
	} else {
		int patchAddr = addr+cp.patchOffset;
		if( cp.newType == 0 ) {
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
	if(size & 2) size += x64Mode();
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
	
cch* def_asmPatch(u64 start, u64 end, char* str)
{
	// start as
	xstr tmpName = tempName("exm");
	FILE* fp = popen(Xstrfmt("as -o %s --%d",
		tmpName.data, ptrSize()), "w");
	if(!fp) return "failed to start as";
	
	// pipe out assembly
	xstr sectNm = xstrfmt(".text$asmPatch_%llX", start);
	fprintf(fp, ".sect \"%s\",\"0\";", sectNm.data);
	fprintf(fp, ".equ @, .-%#I64X;", start);
	for(auto& symb : RngRBL(Linker::
	symbols,Linker::nSymbols)) if(symb.Name
	&&(symb.section == Linker::Type_Relocate))
	fprintf(fp,".equ %s, @+%#I64X;", symb.Name, symb.getAddr());
	fprintf(fp, "%s\n", str);
	if(pclose(fp)) return "as returned with error";

	// load the object
	auto file = loadFile(tmpName);
	if(!file) load_error("object", tmpName);
	Linker::object_load(tmpName, file.data, file.size);
	file.free();
	
	// locate section
	auto sect = Linker::findSection(sectNm);
	if(!sect) return "ASMPATCH section not found";
	if(isNeg(end)) { end = start + sect->length; }
	ei(sect->length > (end-start)) 
		return "ASMPATCH patch too big";
	IFRET(def_memNop(start, end));
	Linker::keepSymbol(sectNm);
	Linker::fixSection(sect, PeFILE::addrToRva64(start));
	remove(tmpName); return 0;
}
