#include "stdafx.h"
#include "exe_mod.h"
#include "def_ops.h"

static FreeLst0 memKeep;

struct MemKeepSeq {
	FreeLst_t* nextPos; u32 end;
	std::pair<u32,u32> init(u32 start, u32 end);
	std::pair<u32,u32> next();
};

std::pair<u32,u32> MemKeepSeq
	::init(u32 start, u32 end)
{
	for(auto& x : memKeep) if(x.rva >= start) { 
		if(x.rva >= end) break; nextPos = &x;
		this->end = end; end = x.rva; 
		if(end == start) return next(); 
		return {start, end}; }
	nextPos = 0; return {start, end};
}

std::pair<u32,u32> MemKeepSeq::next()
{
	// get start position
	if(!nextPos) return {0,0};
	u32 start = nextPos->end;
	u32 end = this->end;
	if(start >= end) return {0,0};
	
	// get end position
	if((++nextPos < memKeep.end())
	&&(end > nextPos->rva)) {
		end = nextPos->rva;
	} else { nextPos = 0; }
	return {start, end};
}

#define MEMKEEP_LOOP(...) {	MemKeepSeq mks; \
	std::tie(start,end) = mks.init(start, end); \
	while(start) { DWORD length = end-start; \
		Void ptr = PeFILE::rvaToPtr(start, length); \
		if(ptr == NULL) return "bad address range"; \
		__VA_ARGS__; std::tie(start,end) = mks.next(); }} 

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
	Linker::Type_Undefined, 0, 0);
	return offset;
}

cch* SymbArg::parseNum(char* str)
{
	return defFileGetNumber(offset, str);
}

cch* SymbArg::parse(char* str)
{
	// parse address
	name = str; offset = 0;
	if(defFileIsAddress(str)) { 
		name = 0; IFRET(parseNum(str)); }
		
	// parse offset
	char* numPos = defFileGetNumPos(str);
	if(numPos && (numPos != str)) { u64 addr = offset;
		IFRET(parseNum(numPos)); offset += addr; 
		*numPos = '\0'; }
	
	return NULL;
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
	MEMKEEP_LOOP(PeFILE::clearSpace(
		start, length, offset)); 
	return 0;
}

cch* def_symbol(u32 rva, char* name)
{
	Linker::addSymbol(name, Linker::Type_Relocate, 
		0, rva); return 0;
}

cch* def_const(u32 value, char* name)
{
	Linker::addSymbol(name, Linker::Type_Absolute,
		0, value); return 0;
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

		Linker::addSymbol(s.name, Linker::Type_Relocate, 0, oldCall);
		
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

cch* def_memKeep(u32 start, u32 end)
{
	DWORD length = end-start;
	Void ptr = PeFILE::rvaToPtr(start, length);
	if(ptr == NULL) return "bad address range";
	memKeep.mark(start, end); 
	return 0;
}

cch* def_memFill8(u32 start, u32 end, int val)
{
	MEMKEEP_LOOP(
		PeFILE::Relocs_Remove(start, length);
		memset(ptr, val, length)); 
	return 0;

}

cch* def_memNop(u32 start, u32 end)
{
	return def_memFill8(start, end, 0x90);
}

cch* def_memTrap(u32 start, u32 end)
{
	return def_memFill8(start, end, 0xCC);
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
		if((size & 4) && x64Mode())
			addr += PeFILE::rvaToAddr(rva)+4;
		if(size & 6){ if(!PeFILE::chkAddrToRva64(addr))
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
		int mode = ((size & 4) && x64Mode()) ? Linker::Type_REL32
			: ((size&1) ? Linker::Type_DIR64 : Linker::Type_DIR32);
		Linker::addReloc(mode, rva, s.symb);
	}
	
	return NULL;
}

SHITCALL cch* def_funcRepl(u32 start, u32 end, char* name)
{
	IFRET(def_freeBlock(start, end, 5));
	return def_makeJump(start, name, false);
}

SHITCALL cch* def_import(char* name, char* symb)
{
	char* dllName = strtok(name, ";");
	char* impName = strtok(NULL, ";");	
	if(!impName) return "IMPORTDEF import name bad";
	
	if(!Linker::addImport(symb, dllName, impName))
		return "IMPORTDEF symbol allready defined";
	return 0;
}

SHITCALL cch* def_export(
	char* name, int iOrd, SymStrArg* frwd)
{
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
	auto* symbol = Linker::addSymbol(frwd ? frwd->name
		: name, Linker::Type_Undefined, 0, 0);
	DWORD symbOffset = frwd ? frwd->offset : 0;
	if(exp->frwd) PeFILE::peExp.setFrwd(*exp, 0);
	Linker::addExport(name, iOrd,
		symbol, symbOffset, exp->rva);
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
	return def_export(name, iOrd, frwd);
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
	
cch* getRegName(int ch)
{
	switch(ch)
	{
	case 'a': return "%eax"; case 'b': return "%ebx";
	case 'c': return "%ecx"; case 'd': return "%edx";
	case 'S': return "%esi"; case 'D': return "%edi";
	default: fatalError("getRegName: %c\n", ch);
	}
}
	

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
	LINKER_ENUM_SYMBOLS(symb, if(symb->Name
		&&(symb->section == Linker::Type_Relocate))
		fprintf(fp,".equ %s, @+%#I64X;", symb->Name, symb->getAddr()); 
	); }
	
	// pipe out assembly body
	cParse cp; if(cp.load2_(str, 0)) 
		return "bad asm input";
	while(auto stmt = cp.tokLst.tok(CTOK_SEMCOL)) {
		if(auto colon = stmt.splitR(CTOK_COLON)) {
			fprintf(fp, "%.*s", colon.text().prn()); }
			
		// check push/pop special
		if((stmt.count() == 1)&&(stmt[0].value() == CTOK_NAME)) {
			auto str = stmt[0].getStr().ptr();
			if(char* regs = strSicmp(str, "ph_")) {
				for(; str.chk(regs); regs++) { fprintf(fp,
				"push %s;", getRegName(*regs)); } goto L1; }
			if(char* regs = strSicmp(str, "pl_")) {
				for(; str.chk(regs);) { fprintf(fp,
				"pop %s;", getRegName(str.ld())); } goto L1; }
		}
			
		// check jump to constant
		if((stmt.count() == 2)&&((stmt[1].getStr().slen > 2))&&
		(stmt[0].value() == CTOK_NAME)&&(stmt[1].value() == CTOK_NUM))
		{ cstr op = stmt[0].getStr(); for(cch* nm : nameJmpCall) 
		if(!op.icmp(nm)){ fprintf(fp, start ? "%.*s %@+%.*s;" : 
		"%.*s @R+(%.*s-@B);", op.prn(), stmt[1].getStr().prn()); goto L1; }}
		
		{ char* tmp = stmt.nTerm();
		fprintf(fp, "%s;", tmp ? tmp : ""); } L1:;
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
	if(!type) type = PeSecTyp::RData;
	Linker::addSection(0, Name, 0, type, align, 0, 0);
	return 0;
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


cch* prologMove_core(Bstr& buff, u32 rva, int prologSz)
{
	Void ptr = PeFILE::rvaToPtr(rva, prologSz);
	if(ptr == NULL) return "bad address range";
	buff.fmtcat(".byte ");
	for(int i = 0; i < prologSz; i++) {
		buff.fmtcat("%d,", ptr[i]); }
	buff.slen--; buff.fmtcat("; jmp 0x%llX", 
		PeFILE::rvaToAddr64(rva+prologSz));	
	return NULL;
}


cch* def_asmSect(cch* name, u32 i, char* str, u32 start)
{
	char sectName[128];
	sprintf(sectName, ".text$%s%X", name, i);
	return def_asmSect(sectName, str, start);
}

cch* def_asmSect2(u32 rva, char* str)
{
	// create section
	char sectName[32]; sprintf(
		sectName, ".text$asmSect%X", rva);
	IFRET(def_asmSect(sectName, str, 0));
	return def_makeJump(rva, sectName, false);
}

cch* def_prologMove(u32 rva, int prologSz, char* name)
{
	Bstr buff; buff.fmtcat(".globl %s; %s:", name, name);
	IFRET(prologMove_core(buff, rva, prologSz));
	return def_asmSect("plgMove",rva,buff,0);
}

cch* def_funcHook(u32 rva, int prologSz, char* name)
{
	IFRET(def_prologMove(rva, prologSz, name));

	Void ptr = PeFILE::rvaToPtr(rva, 5);
	char buff[128]; sprintf(buff, "jmp %s", 
		xstr(Linker::symbcat(name, "_hook")).data);
	return def_asmPatch(rva, -1, buff);
}

// simple reloc killing patch
DEF_RETPAIR(patchChk_t, cch*, err, Void, ptr);
static patchChk_t patchChk (u32 rva, u32 len) {
	Void ptr = PeFILE::rvaToPtr(rva, len);
	if(ptr == NULL) return {"bad patch address", 0};
	PeFILE::Relocs_Remove(rva, len); return {0,ptr}; }
#define PATCH_CHECK(ptr, rva, len) Void ptr; \
	{ auto tmp = patchChk(rva, len); ptr = tmp.ptr; \
	if(!ptr) return tmp.err; }

// jump helpers
static inline
bool i386Jump_chk8(int offset) {
	return inRng(offset+2, -128, 127); }
static inline
void i386Jump_short(Void ptr, int offset) {
	*ptr = 0xEB; ptr.Byte(1) = offset-2; }
static inline
void i386Jump_near(Void ptr, int offset) {
	*ptr = 0xE9; ptr.Dword(1) = offset-5; }
	
static 
cch* makeJump(u32 rva, u32 target)
{
	// prepare patch
	u32 offset = target-rva;
	u32 len = i386Jump_chk8(offset) ? 5 : 2;
	PATCH_CHECK(ptr, rva, len);
	
	// apply patch
	if(len > 2) i386Jump_near(ptr, offset);
	else i386Jump_short(ptr, offset);
	return NULL;
}

cch* def_codeSkip(u32 start, u32 end)
{
	IFRET(def_memFill8(start, end, 0x90));
	if(end-start <= 2) return NULL;
	return makeJump(start, end);
}

cch* def_codeSkip(u32 start)
{
	return def_codeSkip(start, start+5);
}

cch* def_makeJump(u32 rva, char* name, bool call)
{
	SymbArg s; IFRET(s.parse(name));
	
	Void ptr = PeFILE::rvaToPtr(rva, 5);
	if(ptr == NULL) return "bad patch address";
	PeFILE::Relocs_Remove(rva, 5); 
	WRI(PB(ptr), call ? 0xE8 : 0xE9);
	if(s.name) { ptr.dword() = s.symInit();
		Linker::addReloc(Linker::Type_REL32, rva+1, s.symb);
	} else {
		ptr.dword() = PeFILE::addrToRva64(s.offset)-(rva+5);
	}
	
	return NULL;
}

cch* def_codeHook(u32 rva, int prologSz, char* str)
{
	Bstr buff; buff.fmtcat("%s;", str);
	prologMove_core(buff, rva, prologSz);

	char sectName[128];
	sprintf(sectName, ".text$def_codeHook%X", rva);
	IFRET(def_asmSect(sectName, buff, 0));
	return def_makeJump(rva, sectName, false);
}

cch* def_makeRet(DWORD& rva, u32 sz, u64* val)
{
	// clear patch address
	int len = val ? 6 : 1; if(sz) len += 2;
	Void ptr = PeFILE::rvaToPtr(rva, len);
	if(!ptr) return "bad patch address";
	PeFILE::Relocs_Remove(rva, len);
	
	// encode the return
	if(val) { ptr[0] = 0xB8; ptr.
		Dword(1) = *val; ptr += 5;}
	if(sz) { ptr[0] = 0xC2; RW(ptr+1) = sz;
	} else { ptr[0] = 0xC3; } return NULL;
}

cch* def_funcKill(u32 start, u32 end, u64* val)
{
	int len = val ? 6 : 1;
	IFRET(def_freeBlock(start, end, len));	
	Void ptr = PeFILE::rvaToPtr(start, len);
	if(!ptr) return "bad patch address";
	if(val) { ptr[0] = 0xB8; ptr.Dword(1) = *val; 
		ptr += 5;} ptr[0] = 0xC3; return NULL;
}


