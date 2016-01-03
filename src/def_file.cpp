#include <string>

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

SHITSTATIC retpair<DWORD, bool> defFileGetNumber(char* str)
{
	int base = 10;
	for(int i = 0; str[i]; i++)
	  if(isalpha(str[i]))
		base = 16;
	char *end;
	DWORD result = strtoul(str, &end, base);
	return retpair<DWORD, bool>(
		result, str == end);
}

SHITSTATIC bool defFileIsAddress(char* str)
{
	if((*str == '+')||(*str == '-')
	||(!strncmp(str, "0x"))) return true;
    for(int i = 0; str[i]; i++)
	  if(!isxdigit(str[i]))
		  return false;
	return true;
}

SHITSTATIC char* defFileGetNumPos(char* str)
{
	char* numPos = strchr(str, '+');
	if(numPos == NULL)
		numPos = strchr(str, '-');
	return numPos;
}

#include "asmpatch\asmPatch.cpp"

char* symbcat(char* symb, const char* str)
{
	int len = strlen(symb);
	char* result = xmalloc(strlen(str)+len+1);
	char* end = strrchr(symb, '@');
	if(end == NULL) end = symb+len;
	sprintf(result, "%.*s%s%s",  end-symb, symb, str, end);
	return result;
}

struct ParseDefLine
{
	xvector argLst; char* line; int lineNo;
	~ParseDefLine() { argLst.free(); }
	char* argn(int idx) { return
		argLst.dataPtr.ref<char*>(idx); }
	char* arg1; char* arg2; char* arg3; int argcCount;	
	void defBad(int srcLn) { fatal_error(
		"def file bad at line %d (%d)\n", lineNo, srcLn); }

char* initLine(char* line, int lineNo)
{
	this->line = line;
	this->lineNo = lineNo;	
	char* curPos = strchr(line, '(');
	if(curPos == 0) defBad(__LINE__);
	*curPos++ = '\0';
	arg1 = 0; arg2 = 0; arg3 = 0;
	argLst.dataSize = 0;
	return curPos;
}

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
	if(result.b) defBad(__LINE__);
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

void keepSymbol(char* name)
{
	keep_list[keep_count-1] = name;
	xNextAlloc(keep_list, keep_count) = 0;
}

void freeBlock(int offset)
{	
	DWORD start = getNumber(arg1);
	DWORD end = getNumber(arg2);
	DWORD length = end-start;
	Void ptr = PeFile::patchChk(start, length);
	if(ptr == NULL)
		fatal_error("def file: address range at line %d\n", lineNo);
	PeFile::clearSpace(PeFile::addrToRva(start), length, offset);
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
	Void ptr = PeFile::patchChk(start, length);
	if(ptr == NULL)
		fatal_error("def file: address range at line %d\n", lineNo);
	PeFile::Relocs_Remove(PeFile::addrToRva(start), length);
	memset(ptr, 0x90, length);
	return retpair<Void, int>(ptr, length);
}

bool mempatch2_hexData(char* strPos, xvector* data)
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
			data->xnxalloc(1).byte() = value;
		ei(valueLen <= 4)
			data->xnxalloc(2).word() = value;
		else
			data->xnxalloc(4).dword() = value;
	}
	return true;
}

void memPatch2(char* strPos)
{
	// parse string as hex
	xvector data = {0};
	bool wideChar = (strPos[-2] == 'L');
	if((wideChar == false)
	&&( mempatch2_hexData(strPos, NULL))) {
		mempatch2_hexData(strPos, &data);
		goto WAS_MEMPATCH_HEX; }
		
	// parse string as ascii
	while(*strPos++ != '"') {
		DWORD result = strPos[-1];
		if((result == 0)||(*strPos == 0)) defBad(__LINE__);
		if(result == '\\') {
			switch(*strPos++) {
			case 'x': {
				char* end;
				int max = wideChar ? 0xFFFF : 0xFF;
				result = strtoul(strPos, &end, 16);
				if(((strPos) == end)||(result > max))
					defBad(__LINE__);
				strPos = end; }
				break;
			case '0':
				result = 0x00;
				break;
			case '\"':
				result = 0x22;
				break;
			case '\\':
				result = 0x5C;
				break;
			case 'n':
				result = 0x0A;
				break;
			case 'r':
				result = 0x0D;
				break;
			case 't':
				result = 0x09;
				break;
			default:
				defBad(__LINE__);
			}
		}

		if(wideChar == true)
			data.xnxalloc(2).word() = result;
		else 
			data.xnxalloc(1).byte() = result;
	}
	
WAS_MEMPATCH_HEX:
	DWORD start = getNumber(arg1);
	Void ptr = PeFile::patchChk(start, data.dataSize);
	if(ptr == NULL)
		fatal_error("def file: address range at line %d\n", lineNo);
	PeFile::Relocs_Remove(PeFile::addrToRva(start), data.dataSize);
	memcpy(ptr, data.dataPtr, data.dataSize);
	data.free();
}

void memPatch(bool hookMode) 
{
	// process arguments (1&2)
	char* strPos = strchr(arg2, '"');
	if(strPos != NULL)
		return memPatch2(strPos+1);
	bool offsetMode = false;
	if(arg2[0] == '@') {
		offsetMode = true;
		arg2 += 1; }
	DWORD addr = getNumber(arg1);
	Void ptr = PeFile::patchChk(addr, 4);
	if(ptr == NULL)
		fatal_error("def file: bad patch address at line %d\n", lineNo);
		
	// handle hook mode (arg3)
	int rva = PeFile::addrToRva(addr);
	if(hookMode == true) {
		DWORD hookAddr = ptr.dword();
		bool relocate = PeFile::Reloc_Find(rva);
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
			fatal_error("def file: @mode invalid at line %d\n", lineNo);
		PeFile::Relocs_Remove(rva);
		Linker::addReloc(Linker::Type_DIR32, -1, 
			PeFile::addrToRva(addr), symbol);
	}
}

retpair<bool, int>
	callPatchCore(Void ptr, bool absOnly)
{
	bool relative = true;
	int patchOffset = 1;
	
	// 0x25FF: jmp indirect
	if( ptr.word() == 0x25FF ) 
	{ 
		ptr.word() = 0xE990;
		patchOffset = 2; 
	}	

	// 0x15FF: call indirect
	ei( ptr.word() == 0x15FF ) 
	{
		ptr.word() = 0xE890;
		patchOffset = 2; 
	}
	
	// 0x??8B: move ea?, [address]
	ei((ptr.word() & 0xC7FF) == 0x58B)
	{
		int r = (ptr.word() >> 11);
		ptr.word() = 0xB890 + (r << 8);
		relative = false;
		patchOffset = 2; 
	}
	
	// 0x8X0F: conditional
	ei((ptr.word() & 0xF0FF) == 0x800F)
	{
		patchOffset = 2; 
	}
		
	// !0xE8: call relative
	// !0xE9: jump relative	
	ei((absOnly == false)
	&&(ptr[0] != 0xE8 )&&(ptr[0] != 0xE9 ))
	{
		ptr[0] = 0xE9;
	}
	return retpair<bool, int>(
		relative, patchOffset);
}

void callPatch(bool hookMode)
{
	DWORD addr = getNumber(arg1);
	Void ptr = PeFile::patchChk(addr, 5);
	if(ptr == NULL)
		fatal_error("def file: bad patch address at line %d\n", lineNo);
	bool relative = true;
	int patchOffset = 1;
	getpair(relative, patchOffset,
		callPatchCore(ptr, false));
	
	// implement hook mode
	Void patchPtr = ptr+patchOffset;
	if(hookMode == true) {
		if(isAddress(arg2) == true)
			fatal_error("def file: CALLHOOK must be symbol at line %d\n", lineNo);
		if((patchOffset > 1)&&(*ptr != 0x0F))
			fatal_error("def file: CALLHOOK must be relative at line %d\n", lineNo);
		DWORD oldCall = patchPtr.dword()+addr+patchOffset+4;
		Linker::addSymbol(arg2, Linker::Type_Relocate, -1, oldCall);
		arg2 = symbcat(arg2, "_hook");
	} SCOPE_EXIT(if(hookMode == true) free(arg2););
	
	// apply patch
	int rva = PeFile::addrToRva(addr);
	PeFile::Relocs_Remove(rva, patchOffset+4);	
	DWORD symbol = getSymbol(arg2, patchPtr);
	if(symbol != DWORD(-1)) {
		Linker::addReloc(relative ? Linker::Type_REL32 : Linker::Type_DIR32,
			-1, rva+patchOffset, symbol);
	} else {
		int patchAddr = addr+patchOffset;
		int callAddr = getNumber(arg2);
		if( relative == true ) {
			patchPtr.dword() = callAddr-(patchAddr+4);
		} else {
			patchPtr.dword() = PeFile::addrToRva(callAddr);
			PeFile::Relocs_Add(rva+patchOffset);
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
				getNumber(arg1)-PeFile::baseAddr()+2;
		} while(arg1 = strtok(NULL, " \t;"));
	} else {
		if(PeFile::nRelocs == 0)
			fatal_error("def file: IMPORTHOOK requires relocs at %d\n", lineNo);
		char* dllName = arg1;
		char* impName = strtok(NULL, ";");
		if(impName == NULL)
			fatal_error("def file: IMPORTHOOK import name bad at %d\n", lineNo);
		int impRva = PeFile::Import_Find(dllName, impName);
		if(impRva <= 0)
			fatal_error("def file: bad import name at line %d\n", lineNo);
		int impAddr = PeFile::rvaToAddr(impRva);
		for(DWORD reloc : Range(PeFile::relocs, PeFile::nRelocs))
		  if((PeFile::imageData+reloc).dword() == impAddr)
			xNextAlloc(patchList, patchCount) = reloc;
	}
		
	// process patch points
	DWORD symbOffset;
	DWORD symbol = getSymbol(arg2, &symbOffset);
	if(symbol == DWORD(-1))
		fatal_error("def file: IMPORTHOOK requires symbol at line %d\n", lineNo);
	for(DWORD patchPos : Range(patchList, patchCount)) 
	{
		DWORD addr = patchPos+PeFile::baseAddr();
		Void ptr = PeFile::patchChk(addr-2, 6);
		if(ptr == NULL) fatal_error(
			"def file: bad reloc/hook address at line %d\n", lineNo);
		bool relative; int patchOffset;
		getpair(relative, patchOffset,
			callPatchCore(ptr, true));
		if(patchOffset != 2) fatal_error(
			"def file: IMPORTHOOK failed (%X), at line %d\n", addr, lineNo);
		PeFile::Relocs_Remove(patchPos, 4);
		(ptr+2).dword() = symbOffset;
		Linker::addReloc(relative ? Linker::Type_REL32 
			: Linker::Type_DIR32, -1, patchPos, symbol);		
	}
}

void codePatch(void)
{
	// apply patch
	auto block = this->memNop();
	auto sect = Linker::findSection(arg3);
	if(sect == NULL) fatal_error(
		"def file: CODEPATCH bad section name, at line %d\n", lineNo);
	if(sect->length > block.b) fatal_error(
		"def file: CODEPATCH patch too large, at line %d\n", lineNo);
	memcpy(block.a, sect->rawData, sect->length);

	// duplicate section
	char* sectName = xstrcat("@patch_", arg3);
	this->keepSymbol(sectName);
	int iSect = Linker::addSection(sect->fileName, sectName, NULL,
		Linker::nRelocs, sect->nReloc, sect->type, 0, block.a-PeFile::imageData, 0);
	for(int i = 0; i < sect->nReloc; i++) {
		auto& reloc = Linker::relocs[sect->iReloc+i];
		Linker::addReloc(reloc.type, iSect, reloc.offset, reloc.symbol);
	}
}

void importDef(void)
{
	char* dllName = strtok(arg1, ";");
	char* impName = strtok(NULL, ";");
	if(impName == NULL)
		fatal_error("def file: IMPORTDEF import name bad at %d\n", lineNo);
	if(Linker::addImport(arg2, dllName, impName) < 0)
		fatal_error("def file: IMPORTDEF symbol allready defined %d\n", lineNo);
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
	Void srcPtr = PeFile::patchChk(start, length);
	Void dstPtr = PeFile::patchChk(dst, length);
	if((srcPtr == NULL)||(dstPtr == NULL))
		fatal_error("def file: address range at line %d\n", lineNo);
		
	// move block
	void* tmpBuff = xMalloc(length); memcpy(tmpBuff, srcPtr, length);
	memset(srcPtr, 0x90, length); memcpy(dstPtr, tmpBuff, length);
	free(tmpBuff); PeFile::Relocs_Find(PeFile::addrToRva(start), length,
		[&](DWORD& reloc) { reloc += dst-start; });
	
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
		bool relative; int patchOffset;
		getpair(relative, patchOffset,
			callPatchCore(fixupPtr, true));
		if(relative == false) { fatal_error(
			"def file: CODEMOVE fixup must be relative at line %d\n", lineNo); }
		DWORD& fixupRef = (fixupPtr+patchOffset).dword();
		if(target == 0) { fixupRef += start-dst; }
		else { fixupRef = target - (fixDst + patchOffset + 4); }
	}
}

void asmPatch(bool limit)
{
	char* quoteEnd = strchr(arg2+1, '\"');
	if((arg2[0] != '\"') || (quoteEnd == NULL))
	  defBad(__LINE__); quoteEnd[0] = '\0';
	DefFileAsmPatch ctx(lineNo); ctx.run(
		getNumber(arg1), arg2+1);
}

void processLine(char* curPos)
{
	// get arguments
	char* arg; bool argsEnd; 
NEXT_ARGUMENT:
	getpair(arg, argsEnd, defFileGetArg(curPos));
	if(arg != NULL) {
		if(argLst.dataSize < 12) { *(char**)
		(size_t(&arg1)+argLst.dataSize) = arg; }
		argLst.xnxalloc(4).ref<char*>() = arg;
		goto NEXT_ARGUMENT; }
	if(argsEnd == true) return;
	argcCount = argLst.dataSize / 4;
	
	if(this->check("KEEP", 1))
		this->keepSymbol(this->arg1);
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

	else
		defBad(__LINE__);
	lineNo = 0;
}
};

void parse_def_file(char* def_file)
{
	int lineCount;
	char** defLines = loadText(def_file, lineCount);
	if(defLines == NULL)
		fatal_error("unable to load def file\n");
		
	ParseDefLine defLine{};
	for(int i = 0; i < lineCount; i++) {
		char* curPos = defLines[i];
		if(defLine.lineNo == 0) {
			if(is_one_of(*curPos, '\0', ';', '/')) continue;
			curPos = defLine.initLine(defLines[i], i+1); 
		} defLine.processLine(curPos);
	} if(defLine.lineNo != 0)
		fatal_error("def file bad, unexpected end\n");
	free(defLines);
}
