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
	Linker::keepSymbol(xstrdup(name));
	return 0;
}

cch* def_freeBlock(u32 start,
	u32 end, int offset)
{
	DWORD length = end-start;
	Void ptr = PeFILE::rvaToPtr(start, length);
	if(ptr == NULL) return "bad address range";
	PeFILE::clearSpace(start, length, offset);
	return 0;
}

cch* def_symbol(u32 rva, char* name)
{
	Linker::addSymbol(name, Linker::Type_Relocate, 
		-1, rva); return 0;
}

cch* def_const(u32 value, char* name)
{
	Linker::addSymbol(name, Linker::Type_Absolute,
		-1, value); return 0;
}

cch* def_callPatch(u32 rva, SymbArg& s, bool hookMode)
{
	char* hook_name = NULL;
	Void ptr = PeFILE::rvaToPtr(rva, 5);
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
		
		s.name = hook_name = Linker::symbcat(s.name, "_hook");
	} SCOPE_EXIT(free(hook_name));
	
	// apply patch
	PeFILE::Relocs_Remove(rva, cp.patchOffset+4);
	if(s.name) { patchPtr.dword() = s.symInit();
		Linker::addReloc(cp.newType ? Linker::Type_DIR32 : 
			Linker::Type_REL32,	rva+cp.patchOffset, s.symb);		
	} else {
		if( cp.newType == 0 ) {
			patchPtr.dword() = PeFILE::addrToRva64
			(s.offset)-(rva+cp.patchOffset+4);
		} else {
			patchPtr.dword() = s.offset;
			PeFILE::Relocs_Add(rva+cp.patchOffset);
		}
	}
	
	return 0;
}

cch* def_memNop(u32 start, u32 end)
{
	DWORD length = end-start;
	Void ptr = PeFILE::rvaToPtr(start, length);
	if(ptr == NULL) return "bad address range";
	PeFILE::Relocs_Remove(start, length);
	memset(ptr, 0x90, length); return 0;
}

cch* def_patchPtr(u32 rva, SymbArg2& s,
	char* hookSymb, int size)
{
	if(size & 2) size += x64Mode();
	Void ptr = PeFILE::rvaToPtr(rva, (size&1) ? 8 : 4);
	if(!ptr) return "bad patch address";
	
	// handle hook mode
	if(hookSymb) { u64 addr = (size&1) ?
		ptr.ref<u64>() : ptr.dword();
		if(size & 2){ if(!PeFILE::chkAddrToRva64(addr))
			return "hook value invalid as rva";
			def_symbol(addr, hookSymb);
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
	
	return NULL;
}

SHITCALL cch* def_funcRepl(u32 start, u32 end, SymbArg& s)
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

cch* def_fixSect(u32 start, u32 end, char* name)
{
	auto sect = Linker::findSection(name);
	if(!sect) return "section not found";
	if(isNeg(end)) { end = start + sect->length; }
	ei(sect->length > (end-start)) 
		return "section/patch too big";
	IFRET(def_memNop(start, end));
	def_keepSymbol(name);
	Linker::fixSection(sect, start);
	return NULL;
}

const char* nameJmpCall[] = {"JMP", "CALL", "JO", "JNO", "JS", "JNS", "JE", 
	"JZ", "JNE", "JNZ", "JB", "JNAE", "JC", "JNB", "JAE", "JNC", "JBE", "JNA",
	"JA", "JNBE", "JL", "JNGE", "JGE", "JNL", "JLE", "JNG", "JG", "JNLE", "JP",
	"JPE", "JNP", "JPO" };

cch* def_asmSect(char* name, char* str, u32 start)
{
	// start as
	xstr tmpName = tempName("exm");
	FILE* fp = popen(Xstrfmt("as -o %s --%d",
		tmpName.data, ptrSize()), "w");
	if(!fp) return "failed to start as";
	
	// pipe assembly header
	fprintf(fp, ".sect \"%s\",\"0\";", name);
	fprintf(fp, ".equ @B, %#I64X;", PeFILE::baseAddr64());
	if(start) { fprintf(fp, ".equ @, .-%#I64X;",
		PeFILE::rvaToAddr64(start));
	for(auto& symb : RngRBL(Linker::
	symbols,Linker::nSymbols)) if(symb.Name
	&&(symb.section == Linker::Type_Relocate))
	fprintf(fp,".equ %s, @+%#I64X;", symb.Name, symb.getAddr()); }
	
	// pipe out assembly body
	cParse cp; if(cp.load2_(str, 0)) 
		return "bad asm input";
	while(auto stmt = cp.tokLst.tok(CTOK_SEMCOL)) {
		if(auto colon = stmt.splitR(CTOK_COLON)) {
			fprintf(fp, "%.*s", colon.text().prn()); }
			
		// check jump to constant
		if((stmt.count() == 2)&&(stmt[0].value() == CTOK_NAME)
		&&(stmt[1].value() == CTOK_NUM)) { cstr op = stmt[0].getStr();
		for(cch* nm : nameJmpCall) if(!op.icmp(nm)){ fprintf(
			fp, start ? "%.*s %@+%.*s;" : "%.*s @R+(%.*s-@B);",
				op.prn(), stmt[1].getStr().prn()); goto L1; 
		}}
		
		fprintf(fp, "%s;", stmt.nTerm()); L1:;
	}
	
	fprintf(fp, "\n");
	if(pclose(fp)) return "as returned with error";	
	
	// load the object
	auto file = loadFile(tmpName);
	if(!file) load_error("object", tmpName);
	Linker::object_load(tmpName, file.data, file.size);
	file.free(); remove(tmpName); return 0;
}
	
cch* def_asmPatch(u32 start, u32 end, char* str)
{
	// create section
	xstr sectNm = xstrfmt(".text$asmPatch_%X", start);
	IFRET(def_asmSect(sectNm, str, start));
	
	// fixate section
	IFRET(def_fixSect(start, end, sectNm));
	return 0;
}

cch* def_sectCreate(char* Name, int align)
{
	if(Linker::findSection(Name)) return "section exists";
	int type = Linker::sectTypeFromName(Name);
	if(type < 0) type = 2; // .rdata
	Linker::addSection(0, Name, 0, type, align, 0, 0);
}

cch* def_sectInsert(char* Name,
	u32 start, u32 end, DWORD offset)
{
	// check patch range
	DWORD length = end-start;
	Void ptr = PeFILE::rvaToPtr(start, length);
	if(ptr == NULL) return "bad address range";
	
	// append section
	auto sect = Linker::findSection(Name);
	if(!sect) return "section not found";
	if(isNeg(offset)) { offset = 
		sect->length - (offset&INT_MAX); }
	IFRET(Linker::sectGrow(sect, offset, length));
	memcpy(sect->rawData+offset, ptr, length);

	return NULL;
}

NEVER_INLINE
cch* def_sectAppend(char* Name, u32 start, u32 end, DWORD ofs) {
	return def_sectInsert(Name, start, end, ofs|INT_MIN); }

cch* def_sectRevIns(char* Name,
	u32 start, u32 mid, u32 end)
{
	IFRET(def_sectInsert(Name, start, mid, 0));
	return def_sectAppend(Name, mid, end, 0);
}
