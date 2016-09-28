#include "x86ins.cpp"

// jump/call instructions
const char* nameJmpCall[] = {"JMP", "CALL", "JO", "JNO", "JS", "JNS", "JE", 
	"JZ", "JNE", "JNZ", "JB", "JNAE", "JC", "JNB", "JAE", "JNC", "JBE", "JNA",
	"JA", "JNBE", "JL", "JNGE", "JGE", "JNL", "JLE", "JNG", "JG", "JNLE", "JP",
	"JPE", "JNP", "JPO" };
const byte opcodeJmpCall[] = { 0xEB, 0xFF, 0x70, 0x71, 0x78, 0x79, 0x74, 0x74, 0x75,
	0x75, 0x72, 0x72, 0x72, 0x73, 0x73, 0x73, 0x76, 0x76, 0x77, 0x77, 0x7C, 0x7C,
	0x7D, 0x7D, 0x7E, 0x7E, 0x7F, 0x7F, 0x7A, 0x7A, 0x7B, 0x7B };

enum { REG_EAX, REG_ECX, REG_EDX, REG_EBX,
	REG_ESP, REG_EBP, REG_ESI, REG_EDI };
enum { ADDR_NONE, ADDR_IMM, ADDR_REG, ADDR_MEM };
enum { SIZE_NONE, SIZE_BYTE, SIZE_WORD, SIZE_LONG };

struct AsmArg
{
	char* symbol; int offset; byte mode;
	byte size, scale; char base, index;
		
	retpair<int,int> parseReg(char* curPos) {
		static const char* regName[] = {"al", "cl", "dl",
			"bl", "ah", "ch", "dh", "bh" ,"ax", "cx", "dx",
			"bx", "sp", "bp", "si", "di", "eax", "ecx", "edx",
			"ebx", "esp", "ebp", "esi", "edi" };
		for(int i = 0; i < 24; i++) 
		  if(strSicmp(curPos+1, regName[i])) {
			return retpair<int,int>(i&7, i>>3); }
		return retpair<int,int>(-1, 0); }
	int parseReg32(char* curPos) { auto result = parseReg(curPos);
		return (result.b == 2) ? result.a : -1; }
		
	bool parse(char* curPos) 
	{
		symbol = NULL; offset = 0; mode = 0;
		size = 0; base = -1; index = -1; scale = -1;
		if(curPos == NULL) return 1; if(*curPos == '*') {
			curPos++; mode = 0x80; }
		if(*curPos == '%') { auto result = parseReg(curPos);
			if(result.a < 0) return 0; base = result.a; size = result.b+1;
			mode |= ADDR_REG; return 1; }
			
		// parse register index
		if(char* brace = strchr(curPos, '(')) { *brace++ = '\0'; 
			mode |= ADDR_MEM;
			if(char* end = strchr(brace, ')')) *end = '\0'; else return 0;
			char* arg = defFileGetArg(brace);
			if(strchk(arg) && ((base = parseReg32(arg)) < 0)) return 0;
			arg = defFileGetArg(brace);
			if(strchk(arg) && ((index = parseReg32(arg)) < 0)) return 0;
			arg = defFileGetArg(brace);
			if(strchk(arg)) { int tmp = atoi(arg); if(tmp != 0) {
				scale = __builtin_clz(tmp)^31; } else { return 0; } }
		}

		// parse symbol/offset
		if(*curPos == '$') { if(mode > 0) return 0;
			mode = ADDR_IMM; curPos++; } else mode |= ADDR_MEM;
		char* numPos = curPos;
		if(!defFileIsAddress(curPos))
			numPos = defFileGetNumPos(curPos);
		if(numPos) { auto result = defFileGetNumber(numPos);
			if(result.b) return 0; offset = result.a; *numPos = '\0'; }
		if(*curPos) symbol = curPos;
		
		// calculate immdiate size
		if(mode == ADDR_IMM) {
			//if(offset != NULL) 
		
		
		
		}
		return 1;
	}
			
	// reg/mem generation
	struct RegMemInfo { union{ struct{ byte rm; byte sib; }; int rms; };
		union { struct { byte len; byte mod; }; int lmd; };
		RegMemInfo(byte l, byte m, byte r) : len(l), mod(m), rm(r) {}
		RegMemInfo(byte l, byte m, byte r, byte s)
			: len(l), mod(m), rm(r), sib(s) {}};
	RegMemInfo regMemEnc(byte reg) { byte mod = symbol ? 2 :
		(!offset ? 0 : (inRange(offset, -128, 127) ? 1 : 2));
		byte rm = (base | ((mod << 6))) ^ (reg << 3);
		if(mode == ADDR_REG) { rm |= 0xE0; return RegMemInfo(1, mod, rm); }	
		if((base != REG_ESP)&&(index < 0)) { if(base < 0) { rm ^= 0xFA;	return
			RegMemInfo(1, 2, rm); } if((mod == 0)&&(base == REG_EBP)) { rm |= 0x40;
			return 	RegMemInfo(2, mod, rm, 0); } return RegMemInfo(1, mod, rm);
		} else { rm = rm & 0xF8 | 4; return RegMemInfo(2, mod, rm, ((base >= 0) ? base : 4)
			| (((index >= 0) ? index : 4) << 3) | (scale << 6)); }
	}		
};








	
struct DefFileAsmPatch 
{ 
	// Assembly output struct
	struct AsmOutput {
		char* symbol[2]; int offset[2];	char mode[2]; 
		byte type, opLen, totLen; byte opcode[7]; 
		bool skip() { return is_one_of(
			type, TYPE_NONE, TYPE_ALTINST); }};
	enum { TYPE_NONE, TYPE_END, TYPE_LABEL,
		TYPE_INST, TYPE_ALTINST };
	enum { MODE_NONE, MODE_REL8, MODE_REL32,
		MODE_DIR8, MODE_DIR16, MODE_DIR32 };
	static byte modeLen(int x) { static const byte modeLen[] 
		= { 0, 1, 4, 1, 2, 4 }; return modeLen[x]; }	
		
	// Assembly output insertion
	AsmOutput* outAlloc(void) {	return &asmOutput.xNextAlloc(); }
	void initEnd(void) { outAlloc()->type = TYPE_END; }
	void initLabel(char* label) {
		AsmOutput* This = outAlloc(); This->mode[0] = TYPE_LABEL;
		This->opLen = 0; This->totLen = 0; This->symbol[0] = label; }
	byte* initInst(int len, bool alt, char mode1, char* symbol1,
		int offset1, char mode2, char* symbol2, int offset2)
	{	if(inRange(mode1, MODE_REL8, MODE_REL32)) offset1 -= (len+modeLen(mode1));
		byte totLen = len + modeLen(mode1) + modeLen(mode2); AsmOutput* This =
		outAlloc(); *This = AsmOutput{symbol1, symbol2, offset1, offset2,
		mode1, mode2, TYPE_INST+alt, len, totLen}; return This->opcode;
	} byte* initInst(int len, bool alt, char mode1,
		char* symbol1, int offset1) { return initInst(len,
			alt, mode1, symbol1, offset1, MODE_NONE, 0, 0); 
	} void initInst(char* opcode, int len, char mode1, char* symbol1, int offset1) {
		memcpy(initInst(len, false, mode1, symbol1, offset1), opcode, len); }
	
	// high level insertion
	int mapMod(int mod) { return !mod ? 0 : (mod == 1) ? MODE_DIR8 : MODE_DIR32; }
	void initInst(int opcode, int len, AsmArg* arg, int reg) {
		auto rm = arg->regMemEnc(reg); byte* op = initInst( 
			len+rm.len, false, mapMod(rm.mod), arg->symbol, arg->offset);
		*(int*)op = len; *((word*)(op+len)) = rm.rms; }
		
	// label system
	bool labelFirstPass;
	struct Label_t { char* name; int value; };
	xnlist<Label_t> labelLst{};
	int* findLabel(char* label);
	void addLabel(char* label, int value);
	
	DefFileAsmPatch(int ln) : lineNo(ln), instNo(0) { }
	~DefFileAsmPatch() { asmOutput.free(); }
	void run(DWORD addr, char* str);
	bool zeroArg(char* opcode);
	bool jumpCall(char* opcode);
	bool normalOpcode(char* opcode);
	int getFixupValue(AsmOutput& out, int index);
	void collectLabel(int addr);
	bool relaxJumps(int addr);
	char* encodeFixup(char* curPos,
		int addr, AsmOutput& out, int index);
	
	enum AsmModifer { 
		MOD_NEAR = 1, MOD_SHORT = 2 };
	xnlist<AsmOutput> asmOutput{}; 
	int lineNo; int instNo;
	AsmArg args[3];	int nArgs;
	int modifier;
	
	// error handling
	void error(const char* a, const char* b) { fatal_error(
		"defFile,asmPatch (%d,%d): %s %s", lineNo, instNo, a, b); }
	void error(const char* a) { error(a, ""); }
	void badop(char* opcode) {
		error("uknown instruction: ", opcode); }
};

int* DefFileAsmPatch::findLabel(char* label)
{
	if(label[0] == '.') { int symb = Linker::findSymbol(label+1);
		if(!Linker::isUndefSymb(symb)) return NULL; 
		return (int*)&Linker::symbols[symb].value; 
	} else {
		for(auto& l : labelLst) if(!strcmp(l.name, label))
		  return &l.value; error("undefined label: ", label);
	}
}

void DefFileAsmPatch::addLabel(
	char* label, int value)
{
	if(label[0] == '.') { int index = Linker::addSymbol(
		label+1, Linker::Type_Relocate, -1, value);
		if(index < 0) error("duplicate symbol: ", label+1);
	} else {
		for(auto& l : labelLst) if(!strcmp(l.name, label))
			error("duplicate label: ", label);
		labelLst.xNextAlloc() = Label_t{label, value};
	}
}

bool DefFileAsmPatch::jumpCall(char* opcode)
{
	int idx = strifnd(opcode, nameJmpCall);
	if(idx == 0) { if(args->mode & 0x80) error(
		"improper '*', indirect only for jmp/call"); return false; }
	bool callMode = (--idx == 1);
	if(args->mode & 0x80) { args->mode & ~0x80; initInst(
		0xFF, 1,args, callMode ? 2 : 4); return true; }
		
	// encode relative8	
	bool altMode = false; if(callMode) goto RELATIVE_CALL;
	if(!(modifier & MOD_NEAR)) { altMode = true;
	byte* ops = initInst(1, false, MODE_REL8,
		args->symbol, args->offset);
	ops[0] = opcodeJmpCall[idx]; }
	
	// encode relative32
	if(!(modifier & MOD_SHORT)) { RELATIVE_CALL:
	byte* ops = initInst((idx > 1) ? 2 : 1,
		altMode, MODE_REL32, args->symbol, args->offset);
	if(idx == 0) ops[0] = 0xE9; ei(idx == 1) ops[0] = 0xE8;
	else { ops[0] = 0x0F; ops[1] = 0x10 + opcodeJmpCall[idx]; }}
	return true;
}

bool DefFileAsmPatch::normalOpcode(char* opcode)
{
	/* dtermine best matching opcode
	int bestOpcode = -1; int score = 0;
	for(int i = 0; i < ARRAYSIZE(x86InsInfo); i++) {
		if((stricmp(x86InsInfo[i].name, opcode))||
		(x86InsInfo[i].nOps() != nArgs)) continue;
		
		// check for size matches
		for(int j = 0; j < nArgs; j++) { if((args[j].size 
			&& args[j].size != x86InsInfo[i].size))
	} */
	
	
	if(!stricmp(opcode, "nop")) { initInst(1,
		false, MODE_NONE, 0, 0)[0] = 0x90; return true; }
	return false; 
}

int DefFileAsmPatch::getFixupValue(AsmOutput& out, int index)
{
	char* label = out.symbol[index];
	if(label == NULL) return out.offset[index];
	int* symbol = findLabel(label);
	if(symbol == NULL) return -1;
	return out.offset[index] + *symbol;
}

void DefFileAsmPatch::collectLabel(int addr)
{
	for(auto& out : asmOutput){ 
		if(out.skip()) continue;
		if(out.type == TYPE_LABEL) {
			if(labelFirstPass) addLabel(out.symbol[0], addr);
			else *findLabel(out.symbol[0]) = addr;
		} addr += out.totLen;
	} labelFirstPass = false;
}

bool DefFileAsmPatch::relaxJumps(int addr)
{
	bool relaxed = false;
	for(int i = 0; i < asmOutput.count-1; i++) { 
		if(asmOutput[i].skip()) continue;
		if((asmOutput[i].type == TYPE_INST)
		&&(asmOutput[i+1].type == TYPE_ALTINST)) {
			int target = getFixupValue(asmOutput[i], 0);
			if((target < 0)||(!inRange(target-addr, -128, 127))) { relaxed = 1;
			asmOutput[i].type = TYPE_NONE; asmOutput[++i].type = TYPE_INST; }
		} addr += asmOutput[i].totLen;
	} return relaxed;
}

char* DefFileAsmPatch::encodeFixup(char* curPos,
	int addr, AsmOutput& out, int index)
{
	if(out.mode[index] == MODE_NONE) return curPos;
	int offset = getFixupValue(out, index);
	if(offset < 0) {
		if(!is_one_of(out.mode[index], MODE_REL32, MODE_DIR32))
		  error("invalid fixup for undefined extern symbol: ", out.symbol[index]);
		int symb = Linker::addSymbol(out.symbol[index]+1, Linker::Type_Undefined, 0, 0);
		Linker::addReloc(out.mode[index] == MODE_REL32 ? Linker::Type_REL32 
			: Linker::Type_DIR32, -1, curPos-PeFile::imageData, symb);
		*(u32*)curPos = 0; return curPos+4;
	} else {
		if(out.mode[index] == MODE_DIR8) { *(u8*)curPos = offset; return curPos+1; }
		if(out.mode[index] == MODE_DIR16) { *(u16*)curPos = offset; return curPos+2; }
		if(out.mode[index] == MODE_DIR32) { *(u32*)curPos = offset; return curPos+4; }
		printf("%X, %X\n", offset, addr);
		
		offset -= addr;
		if(out.mode[index] == MODE_REL8) { if(!inRange(offset, -128, 127))
			error("range exceeded for short jump");
			*(u8*)curPos = offset; return curPos+1;
		} else { *(u32*)curPos = offset; return curPos+4; }
	}
}

void DefFileAsmPatch::run(DWORD addr, char* str)
{
	for(char* curPos = strtok(str, ";");
	  instNo++, curPos; curPos = strtok(NULL, ";"))
	{
		// parse label/opcode
		char* opcode = defFileGetArg(curPos, true);
		int opcodeLen = strlen(opcode);
		if(opcode[opcodeLen-1] == ':') {
			opcode[opcodeLen-1] = '\0'; initLabel(opcode);
			opcode = defFileGetArg(curPos, true);
			if(opcode == NULL) continue; }
			
		// handle modifiers
		modifier = 0; char* modPos;
		char* arg1 = defFileGetArg(curPos, false);
		if(arg1 == NULL) goto NO_MODIFIER;
		if(modPos = strSicmp(arg1, "NEAR")) modifier |= MOD_NEAR; 
		ei(modPos = strSicmp(arg1, "SHORT")) modifier |= MOD_SHORT;
		else { goto NO_MODIFIER; } arg1 = defFileGetArg(modPos, false);
	NO_MODIFIER:
	
		// parse arguments
		this->nArgs = 0;
		while((nArgs < 3) && (arg1 != NULL)) {
		  if(!args[nArgs++].parse(arg1))
			this->error("bad operand");
		  arg1 = defFileGetArg(curPos);
		}
		
		// assemble instructions
		if(jumpCall(opcode)) continue;
		if(normalOpcode(opcode)) continue;
		this->error("bad opcode", opcode);
		
	} initEnd();
	
	// get output length
	labelFirstPass = true;
	while(collectLabel(addr), relaxJumps(addr));
	DWORD length = 0;
	for(auto& out : asmOutput)
      if(out.type == TYPE_INST)
	    length += out.totLen;
	PeFile::Relocs_Remove(PeFile::addrToRva(addr), length);
	
	// write output data
	char* curPos = PeFile::patchChk(addr, length);
	for(auto& out : asmOutput) {
	  if(out.type == TYPE_INST) { memcpy(curPos, out.opcode, out.opLen);
		curPos = encodeFixup(encodeFixup(curPos + 
			out.opLen, addr, out, 0), addr, out, 1); 
		addr += out.totLen; }
	}
}
