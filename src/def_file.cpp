#include <string>
#include "def_ops.h"

struct Args {
	
};


SHITSTATIC retpair<char*, bool> defFileGetArg(
	char*& curPos, bool ss = false)
{
	// locate argument start
	while(inRange(*curPos, 1, ' ')) curPos++;
	if(*curPos == '\0') return retpair<char*, bool>(0, 0);
	retpair<char*, bool> result(curPos, 0);
	
	// locate argument end
	char* endPos; bool argsEnd = false;
	int braceLevel = 0; char strState = 0;
	while(char ch = *(endPos = curPos)) { curPos++;
		if(strState != 0) {
			if(strState > 1) { strState = 1; }
			ei(ch == '\\') { strState = 2; }
			ei(ch == '\"') { strState = 0; }
		} ei(ch == '(') braceLevel++;
		ei(braceLevel) { if(ch == ')') braceLevel--; }
		ei((ch == ',')||((ch == ' ') && ss)) { break; }
		ei(ch == ')') { result.b = true; break; }		
	} do { *endPos-- = '\0'; }
	while((result.a <= endPos) 
	&& inRange(*endPos, 1, ' '));
	return result; 
}

SHITCALL cch* defFileGetNumber(u64& out, char* str)
{
	int base = 10;
	for(int i = 0; str[i]; i++)
	  if(isalpha(str[i]))
		base = 16;
	char *end;
	out = _strtoui64(str, &end, base);
	return (str == end) ? "bad number" : 0;
}

SHITCALL bool defFileIsAddress(char* str)
{
	if((*str == '+')||(*str == '-')
	||(!strncmp(str, "0x"))) return true;
    for(int i = 0; str[i]; i++)
	  if(!isxdigit(str[i]))
		  return false;
	return true;
}

SHITCALL char* defFileGetNumPos(char* str)
{
	char* numPos = strchr(str, '+');
	if(numPos == NULL)
		numPos = strchr(str, '-');
	return numPos;
}

#if 0

#include "asmpatch\asmPatch.cpp"

struct ParseDefLine
{
	cParse cp; char* line; int argcCount;
	//cParse::Parse_t arg1, arg2, arg3;
	char* arg1; char* arg2; char* arg3;
	xVector<cParse::Parse_t> argLst;
	//bool hasAtSym(cParse::Parse_t& arg);
	
	void defBad(int srcLn, char* err) { fatal_error(
		"def file:%d:%d: bad (%d)", cp.getLine(err), srcLn); }
	void defBad(cch* str, char* err) { fatal_error(
		"def file:%d:%d: %s", cp.getLine(err), str); }
	char* argn(int idx) { return argLst[idx]->str; }
		
	//char* arg1; char* arg2; char* arg3; int argcCount;	
	//void defBad(int srcLn) { fatal_error(
	//	"def file bad at line %d (%d)\n", lineNo, srcLn); }

bool check(const char* cmd, int argc) 
{
	if(stricmp(line, cmd))
		return false;
	if(min(3,argcCount) != argc)
		return false;
	return true; 
}

DWORD getNumber(char* str) {
	auto result = defFileGetNumber(str);
	if(result.b) defBad("bad number", str);
	return result.a; }
bool isAddress(char* str)
{	return defFileIsAddress(str); }

DWORD getSymbol(char* str, DWORD* offset) 
{
	if(isAddress(str))
		return DWORD(-1);
		
	// get offset
	*offset = 0;
	char* numPos = defFileGetNumPos(str);
	if(numPos != NULL) { *offset = getNumber(numPos);
		*numPos = '\0'; }
	
	// create symbol
	return Linker::addSymbol(str, 
		Linker::Type_Undefined, -1, 0);
}

void freeBlock(int offset)
{	
	DWORD start = getNumber(arg1);
	DWORD end = getNumber(arg2);
	DWORD length = end-start;
	Void ptr = PeFILE::patchChk(start, length);
	if(ptr == NULL)
		defBad("bad address range", arg1);
	PeFILE::clearSpace(PeFILE::addrToRva(start), length, offset);
}

void symbol(bool relocate) 
{
	DWORD value = getNumber(arg1);
	Linker::addSymbol(arg2, relocate ? Linker::Type_Relocate
		: Linker::Type_Absolute, -1, value); 
}

retpair<Void, int> memNop(void)
{
	DWORD start = getNumber(arg1);
	DWORD end = getNumber(arg2);
	DWORD length = end-start;
	Void ptr = PeFILE::patchChk(start, length);
	if(ptr == NULL)
		defBad("bad address range", arg1);
	PeFILE::Relocs_Remove(PeFILE::addrToRva(start), length);
	memset(ptr, 0x90, length);
	return retpair<Void, int>(ptr, length);
}

bool mempatch2_hexData(char* strPos, xvector_* data)
{
	while(*strPos++ != '"') {
		if(strPos[-1] != '\\')
			return false;
		char* end;
		DWORD value = strtoul(strPos, &end, 16);
		DWORD valueLen = end-strPos;
		strPos = end;
		if((valueLen-1) > 7)
			return false;
		if(data == NULL)
			continue;
		if(valueLen <= 2)
			data->xnxalloc_(1).byte() = value;
		ei(valueLen <= 4)
			data->xnxalloc_(2).word() = value;
		else
			data->xnxalloc_(4).dword() = value;
	}
	return true; 
}

void memPatch2(char* strPos)
{
	// parse string as hex
	xvector_ data = {0};
	bool wideChar = (*strPos == 'L');
	strPos += wideChar ? 2 : 1;
	if((wideChar == false)
	&&( mempatch2_hexData(strPos, NULL))) {
		mempatch2_hexData(strPos, &data);
		goto WAS_MEMPATCH_HEX; }
		
	// parse string as ascii
	while(1) { byte ch = RDI(strPos);
		DWORD chw; if(ch == '"') break;
		
		// escape character
		if(ch == '\\') { switch(RDI(strPos)) {
		case 'x': { char* end; chw = strtoul(strPos, &end, 16);
			if(strPos == end) defBad("bad hex value", end);
			strPos = end;  goto SKIP_WIDEN; }
		case '0': ch = 0x00; break;	case '\"': ch = 0x22; break;
		case '\\': ch = 0x5C; break; case 'n': ch = 0x0A; break;
		case 'r': ch = 0x0D; break;	case 't': ch = 0x09; break;
		default: defBad("bad escape code", strPos); }}
	
		// append character
		chw = ch; SKIP_WIDEN:
		if(wideChar == true)
			data.xnxalloc_(2).word() = chw;
		else 
			data.xnxalloc_(1).byte() = chw;
	}
	
WAS_MEMPATCH_HEX:
	DWORD start = getNumber(arg1);
	Void ptr = PeFILE::patchChk(start, data.dataSize);
	if(ptr == NULL) defBad("bad patch address", arg1);
	PeFILE::Relocs_Remove(PeFILE::addrToRva(start), data.dataSize);
	memcpy(ptr, data.dataPtr, data.dataSize);
	data.free();
}

void memPatch(bool hookMode) 
{
	// check bytes mode
	if((arg2[0] == '"')
	||(RW(arg2) == '"L'))
		return memPatch2(arg2);
		
	
	// process arguments (1&2)	
	bool offsetMode = false;
	if(arg2[0] == '@') {
		offsetMode = true;
		arg2 += 1; }
	DWORD addr = getNumber(arg1);
	Void ptr = PeFILE::patchChk(addr, 4);
	if(ptr == NULL)
		defBad("bad patch address", arg1);
		
	// handle hook mode (arg3)
	int rva = PeFILE::addrToRva(addr);
	if(hookMode == true) {
		DWORD hookAddr = ptr.dword();
		bool relocate = PeFILE::Reloc_Find(rva);
		Linker::addSymbol(arg3, relocate ? Linker::Type_Relocate
		: Linker::Type_Absolute, -1, hookAddr); }

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

void callPatch(bool hookMode)
{
	DWORD addr = getNumber(arg1);
	Void ptr = PeFILE::patchChk(addr, 5);
	if(ptr == NULL)
		defBad("bad patch address", arg1);
	auto cp = callPatchCore(ptr, true);
	
	// implement hook mode
	Void patchPtr = ptr+cp.patchOffset;
	if(hookMode == true) {
		if(isAddress(arg2) == true) defBad("CALLHOOK must be symbol", arg2);
		if(cp.addrType != 2) defBad("CALLHOOK must be relative", arg1);
		DWORD oldCall = patchPtr.dword()+addr+cp.patchOffset+4;
		Linker::addSymbol(arg2, Linker::Type_Relocate, -1, oldCall);
		arg2 = Linker::symbcat(arg2, "_hook");
	} SCOPE_EXIT(if(hookMode == true) free(arg2););
	
	// apply patch
	int rva = PeFILE::addrToRva(addr);
	PeFILE::Relocs_Remove(rva, cp.patchOffset+4);	
	DWORD symbol = getSymbol(arg2, patchPtr);
	if(symbol != DWORD(-1)) {
		Linker::addReloc(cp.relative ? Linker::Type_REL32 : 
			Linker::Type_DIR32,	rva+cp.patchOffset, symbol);
	} else {
		int patchAddr = addr+cp.patchOffset;
		int callAddr = getNumber(arg2);
		if( cp.relative == true ) {
			patchPtr.dword() = callAddr-(patchAddr+4);
		} else {
			patchPtr.dword() = PeFILE::addrToRva(callAddr);
			PeFILE::Relocs_Add(rva+cp.patchOffset);
		}
	}
}

void importHook(void)
{
	// get list of patch points
	DWORD* patchList = NULL;
	int patchCount = 0;	
	arg1 = strtok(arg1, ";");
	if(isAddress(arg1) == true) {
		do {
			xNextAlloc(patchList, patchCount) = 
				getNumber(arg1)-PeFILE::baseAddr()+2;
		} while(arg1 = strtok(NULL, " \t;"));
	} else {
#if 0
		if(PeFILE::nRelocs == 0)
			defBad("IMPORTHOOK requires relocs", arg1);
		char* dllName = arg1;
		char* impName = strtok(NULL, ";");
		if(impName == NULL)
			defBad("IMPORTHOOK import name bad", arg1);
		int impRva = PeFILE::Import_Find(dllName, impName);
		if(impRva <= 0)
			defBad("IMPORTHOOK import name bad", arg1);
		int impAddr = PeFILE::rvaToAddr(impRva);
		for(DWORD reloc : Range(PeFILE::relocs, PeFILE::nRelocs))
		  if((PeFILE::imageData+reloc).dword() == impAddr)
			xNextAlloc(patchList, patchCount) = reloc;
#endif
	}
		
	// process patch points
	DWORD symbOffset;
	DWORD symbol = getSymbol(arg2, &symbOffset);
	if(symbol == DWORD(-1))
		defBad("IMPORTHOOK requires symbol", arg2);
	for(DWORD patchPos : Range(patchList, patchCount)) 
	{
		DWORD addr = patchPos+PeFILE::baseAddr();
		int ofs = 2;
	RETRY_PATCH:
		Void ptr = PeFILE::patchChk(addr-ofs, 6);
		if(ptr == NULL) defBad("bad reloc/hook address", arg2);
		auto cp = callPatchCore(ptr, false);	
		if(cp.addrType != 1) { if(--ofs) goto RETRY_PATCH; 
			defBad(xstrfmt("IMPORTHOOK failed (%X)", addr), arg2); }
		PeFILE::Relocs_Remove(patchPos, 4);
		(ptr+ofs).dword() = symbOffset;
		Linker::addReloc(cp.relative ? Linker::Type_REL32 
			: Linker::Type_DIR32, patchPos, symbol);
	}
}

void codePatch(void)
{
	// apply patch
	auto block = this->memNop();
	auto sect = Linker::findSection(arg3);
	if(sect == NULL) defBad("CODEPATCH bad section name", arg3);
	if(sect->length > block.b) 
		defBad("CODEPATCH patch too large", arg3);
	memcpy(block.a, sect->rawData, sect->length);

	// duplicate relocs
	u32 blockRva = PeFILE::ptrToRva(block.a);
	for(auto& reloc : Range(sect->relocs, sect->nReloc)) {
		Linker::addReloc(reloc.type, reloc.
			offset+blockRva, reloc.symbol);
	}
}

void importDef(void)
{
	char* dllName = strtok(arg1, ";");
	char* impName = strtok(NULL, ";");
	if(impName == NULL) defBad("IMPORTDEF import name bad", arg1);
	if(Linker::addImport(arg2, dllName, impName) < 0)
		defBad("IMPORTDEF symbol allready defined", arg2);
}

void funcRepl(void)
{
	this->freeBlock(5);
	arg2 = arg3;
	this->callPatch(false);
}

void codeMove(void)
{
	// process arguments
	DWORD start = getNumber(arg1);
	DWORD dst = (arg3[0] != '@') ? 	getNumber(arg3) 
		: start + getNumber(arg3+1);
	DWORD length = getNumber(arg2)-start;
	Void srcPtr = PeFILE::patchChk(start, length);
	Void dstPtr = PeFILE::patchChk(dst, length);
	if((srcPtr == NULL)||(dstPtr == NULL))
		defBad("address range bad", arg1);
		
	// move block
	void* tmpBuff = xMalloc(length); memcpy(tmpBuff, srcPtr, length);
	memset(srcPtr, 0x90, length); memcpy(dstPtr, tmpBuff, length);
	free(tmpBuff); PeFILE::Relocs_Move(PeFILE::
		addrToRva(start), length, dst-start);
	
	// perform fixups
	for(int i = 3; i < argcCount; i++)
	{
		// get fixup arguments
		DWORD target = 0;
		if(char* colonPos = strchr(argn(i), ':'))
			target = getNumber(colonPos);
		DWORD fixup = getNumber(argn(i));
		DWORD fixDst = fixup + dst-start;
		Void fixupPtr = dstPtr + (fixup-start);
		
		// apply fixup to short jump
		/*if((*fixupPtr == 0xEB)||
		((*fixupPtr & 0xF0) == 0x70)) {	if(target == 0) 
			target = (fixup+2) + fixupPtr.ref<char>();
			int offset = */
			
		// apply fixup to near jump
		auto cp = callPatchCore(fixupPtr, false);
		if(cp.addrType != 2) defBad("CODEMOVE fixup must be relative", argn(i));
		DWORD& fixupRef = (fixupPtr+cp.patchOffset).dword();
		if(target == 0) { fixupRef += start-dst; }
		else { fixupRef = target - (fixDst + cp.patchOffset + 4); }
	}
}

void asmPatch(bool limit)
{
	char* quoteEnd = strchr(arg2+1, '\"');
	if((arg2[0] != '\"') || (quoteEnd == NULL))
	  defBad(__LINE__, arg2); quoteEnd[0] = '\0';
	DefFileAsmPatch ctx(cp); ctx.run(
		getNumber(arg1), arg2+1);
}

void exportDef(bool hasArg2)
{
	// parse export name
	char* name = strtok(arg1, ";: "); 
	char* ord = strtok(NULL, ";: ");
	if(is_one_of(*name, '#', '@')) 
		ord = release(name)+1;
	u32 iOrd = ord ? getNumber(is_one_of(
		*ord, '#', '@') ? ord+1 : ord) : 0;
		
	// lookup export
	auto exp = PeFILE::peExp.find(name, iOrd); if(exp.err) {
		if(exp.err > 0)	defBad("EXPORTDEF: name ordinal missmatch", arg1);
		defBad("EXPORTDEF: mistake catcher; existing export specified "
			"by ordinal when name exists", arg1); }
	if(!exp) { exp.slot = &PeFILE::peExp.add(name, iOrd); }
	
	// handle immidiate, forwarder
	if(hasArg2){ if(*arg2 == '"') { PeFILE::peExp
		.setFrwd(*exp, strtok(arg2, "\"")); return; }
	if(isAddress(arg2)) { PeFILE::peExp.setRva(*exp, 
		PeFILE::addrToRva(getNumber(arg2))); return; }}
		
	// handle symbol
	DWORD symbOffset; DWORD symbol = getSymbol(
		hasArg2 ? arg2 : name, &symbOffset);
	if(exp->frwd) PeFILE::peExp.setFrwd(*exp, 0);
	Linker::addExport(name, iOrd, 
		symbol, symbOffset, exp->rva);
}

void processLine(void)
{
	if(this->check("KEEP", 1))
		Linker::keepSymbol(this->arg1);
	ei(this->check("FREE", 2))
		this->freeBlock(0);
	ei(this->check("CONSTANT", 2))
		this->symbol(false);
	ei(this->check("SYMBOL", 2))
		this->symbol(true);
	ei(this->check("MEMNOP", 2))
		this->memNop();
	ei(this->check("MEMPATCH", 2))
		this->memPatch(false);
	ei(this->check("MEMHOOK", 3))
		this->memPatch(true);
	ei(this->check("CALLPATCH", 2))
		this->callPatch(false);
	ei(this->check("CALLHOOK", 2))
		this->callPatch(true);
	ei(this->check("IMPORTHOOK", 2))
		this->importHook();
	ei(this->check("CODEPATCH", 3))
		this->codePatch();
	ei(this->check("IMPORTDEF", 2))
		this->importDef();
	ei(this->check("FUNCREPL", 3))
		this->funcRepl();
	ei(this->check("CODEMOVE", 3))
		this->codeMove();
	ei(this->check("ASMPATCH", 2))
		this->asmPatch(false);
	ei(this->check("ASMPATCH", 3))
		this->asmPatch(true);
	ei(this->check("EXPORTDEF", 1))
		exportDef(false);
	ei(this->check("EXPORTDEF", 2))
		exportDef(true);
	else
		defBad("bad command", line);
}
};

#endif

struct ParseDefLine
{
	// parsing state
	cParse cp; char* funcName; 
	xVector<cParse::Parse_t> argLst;
	char*& argStr(int i) { return 
		argLst.dataPtr.ref<char*>(i); }
	int argc() { return argLst.dataSize; }

	// argument definition
	struct ArgDef { int val; byte get(int i) { return val>>((i+1)*8); } 
	int count() { return val&127; } bool va() { return s8(val) < 0; }
	ALWAYS_INLINE int vi(byte a, int i) { return (s8(a)<0)
		? 0x80 : (a ? (a<<((i+1)*8))|1 : 0); }
	ALWAYS_INLINE ArgDef(byte a1, byte a2=0, byte a3=0, byte a4=0) 
		: val(vi(a1,0)+vi(a2,1)+vi(a3,2)+vi(a4,3)) { } };
	enum { None, Num, Raw, SyN, SyN2, SyS, VArg = -1 };
	
	// argument data
	union Arg_t { u64 num;  char* raw; 
		SymbArg syn; SymbArg2 syn2; SymStrArg sys;
		TMPL(T) operator T&() { return *(T*)this; } 
		Void v() { return Void(this); }};
	Arg_t a1, a2, a3; xarray<char*> va;
	Arg_t& argn(int i) { return (&a1)[i]; }
	
	void defBad(int srcLn, char* err) { fatal_error(
		"def file:%d:%d: bad (%d)", cp.getLine(err), srcLn); }
	void defBad(cch* str, char* err) { fatal_error(
		"def file:%d:%d: %s", cp.getLine(err), str); }
		
	bool check(cch* name, ArgDef argDef);
		
	cch* processLine();
};

void parse_def_file(
	char* def_file, void* data)
{
	// load def file
	ParseDefLine defLine{};
	char* err = defLine.cp.load2_(
		data, cParse::FLAG_STRCOMBINE);
	if(err) defLine.defBad("load failed", err);
	
	// parse def file
	defLine.argLst.xReserve(3); while(1) {
	cstr str = defLine.cp.tokLst.getCall(defLine.argLst);
	if(!str.slen) { if(str.data) defLine.defBad(
		__LINE__, str.data); break; }
		
	// convert args array to string
	int argc = defLine.argLst.getCount();
	for(int i = 0; i < argc; i++) { defLine
		.argStr(i) = defLine.argLst[i].nTerm(); }
	defLine.argLst.dataSize = argc;
	
	defLine.funcName = str.nterm();
	defLine.processLine(); } 
		
		
		
	/*defLine.argcCount = defLine.argLst.getCount();
	for(int i = 0; i < defLine.argLst.getCount(); i++) {
		char* str = defLine.argLst[i].nTerm(); 
		if(i < 3) (&defLine.arg1)[i] = str; }*/
	
}

bool ParseDefLine::check(cch* name, ArgDef argDef)
{
	// match arguments
	if(stricmp(funcName, name)) return false;
	int argcDef = argDef.count();
	
	printf("%d, %d\n", argc(), argcDef);
	
	if(argDef.va()) { if(argc() < argcDef) return false;
	} else { if(argc() != argcDef) return false; }
		
	// parse the arguments
	for(int i = 0; i < argcDef; i++) {
	char *str = argStr(i);
	switch(argDef.get(i)) {
	case Num: { 
		cch* err = defFileGetNumber(
			argn(i).num, str); break; }
	case SyN: {
		cch* err = argn(i).syn2.parse(str);
		if(err) defBad("bad number", str); break; }
	case SyN2: {
		cch* err = argn(i).syn2.parse(str);
		if(err) defBad("bad number", str); break; }
	case SyS: {
		cch* err = argn(i).sys.parse(str);
		if(err) defBad("bad number", str); break; }	
		
	case Raw:
		printf("%s\n", str);
	
		argn(i).raw = str; break;
	}}
	
	// setup varargs
	va.data = &argStr(argcDef);
	va.len = argc()-argcDef;
		
	return true; 
}


cch* ParseDefLine::processLine()
{
	#define FUNC(fn,ad,func) if(check(fn,ad)) { return func; }

	FUNC("KEEP", ArgDef(Raw), def_keepSymbol(a1));
	FUNC("FREE", ArgDef(Num,Num), def_freeBlock(a1,a2,0));
	FUNC("CONSTANT", ArgDef(Num,Raw), def_const(a1,a2));
	FUNC("SYMBOL", ArgDef(Num,Raw), def_symbol(a1,a2));
	FUNC("CALLPATCH", ArgDef(Num,SyN), def_callPatch(a1,a2,0));
	FUNC("CALLHOOK", ArgDef(Num,SyN), def_callPatch(a1,a2,1));
	FUNC("MEMNOP", ArgDef(Num,Num), def_memNop(a1,a2));	
	FUNC("FUNCREPL", ArgDef(Num,Num,SyN), def_funcRepl(a1,a2,a3));

	// import/export functions
	FUNC("IMPORTDEF", ArgDef(Raw), def_import(a1,0))
	FUNC("IMPORTDEF", ArgDef(Raw,Raw), def_import(a1,a2))
	FUNC("EXPORTDEF", ArgDef(Raw), def_export(a1,0))
	FUNC("EXPORTDEF", ArgDef(Raw,SyS), def_export(a1,a2.v()))
	
	// ptr patch functions
	FUNC("PATCH_PTR", ArgDef(Num,SyN2), def_patchPtr(a1,a2,0,2));
	FUNC("PATCH_PTR", ArgDef(Num,SyN2,Raw), def_patchPtr(a1,a2,a3,2));
	FUNC("PATCH_I32", ArgDef(Num,SyN2), def_patchPtr(a1,a2,0,0));
	FUNC("PATCH_I32", ArgDef(Num,SyN2,Raw), def_patchPtr(a1,a2,a3,0));
	FUNC("PATCH_I64", ArgDef(Num,SyN2), def_patchPtr(a1,a2,0,1));
	FUNC("PATCH_I64", ArgDef(Num,SyN2,Raw), def_patchPtr(a1,a2,a3,1));
}
