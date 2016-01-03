
struct ImportSlot
{
	bool HasName() {
		return name && (name[0] != '#'); }
	const char* Name() {
		return this->name; }
	DWORD Hint() {
		return hint; }
	void init(const char* name, DWORD hint) {
		if(name == NULL) { this->name = xMalloc(8);
			sprintf(this->name, "#%d", (WORD)hint); }	
		else { if(name && (name[0] == '#')) hint = atoi(name+1);
			this->name = xstrdup(name); }
		this->hint = hint; }
	void free() {
		:: free(name); }
private:
	char* name;
	DWORD hint;
};

struct ImportDir
{
	char* DllName;
	DWORD TimeDateStamp;
	DWORD FirstThunk;
	ImportSlot* imports;
	int nImports;
	
	retpair<PeBlock*, int> getBlocks(
		int nImports, int* iatIndex);
	IMAGE_IMPORT_DESCRIPTOR* rebuild(PeBlock* blocks,
		int nBlocks, Void imgBase, int nImports);
	int thunkLen(void) {
		return (nImports+1)*4; }
	
	struct UserData { 
		BYTE type; BYTE dllIndex;
		WORD slotIndex; 
		operator size_t() {
			return *(size_t*)this; }};
	enum {
		Type_ImpDir, Type_NameLst,
		Type_DllName, Type_ImpName,
		Type_FirstThunk };
};

ImportDir* imports;
int nImports;

retpair<PeBlock*, int> ImportDir::
	getBlocks(int nImports, int* iatIndex)
{
	// allocate block list
	int nBlocks = nImports*2+1;
	for(auto& dir : Range(this, nImports)) {
		nBlocks += (dir.FirstThunk == 0);
		for(auto& slot : Range(dir.imports, dir.nImports))
			nBlocks += (slot.HasName() != 0); }
	PeBlock* blocks = xMalloc(nBlocks+10);
	for(int i = 0; i < nBlocks; i++) {
		blocks[i].type = Type_RData;
		blocks[i].align = 1; }
		
	// import directory table
	PeBlock* curBlock = blocks;
	curBlock->userData = UserData{Type_ImpDir, 0, 0};
	curBlock->length = (nImports+1) *
		sizeof(IMAGE_IMPORT_DESCRIPTOR);
	curBlock->align = 4; curBlock++;
	
	// import directory names
	for(int i = 0; i < nImports; i++) {
		curBlock->userData = UserData{Type_DllName, i, 0};
		curBlock->length = strlen(this[i].DllName)+1;
		curBlock++; }

	// original first thunks
	for(int i = 0; i < nImports; i++) {
		curBlock->userData = UserData{Type_NameLst, i, 0};
		curBlock->length = (this[i].nImports+1)*4;
		curBlock->align = 4; curBlock++; }

	// import function names
	for(int i = 0; i < nImports; i++) {
		for(int j = 0; j < this[i].nImports; j++)
		if(this[i].imports[j].HasName() != 0) {
			curBlock->userData = UserData{Type_ImpName, i, j};
			curBlock->length = strlen(this[i].imports[j].Name())+3;
			curBlock++; }
	}
	
	// import address tables
	if(iatIndex != NULL)
		*iatIndex = curBlock-blocks;
	for(int i = 0; i < nImports; i++)
	if(this[i].FirstThunk == 0) {
		curBlock->userData = UserData{Type_FirstThunk, i, 0};
		curBlock->length = (this[i].nImports+1)*4;
		curBlock->align = 4; curBlock->type = Type_Data;
		curBlock++; }
	return retpair<PeBlock*, int> (blocks, nBlocks);
}

IMAGE_IMPORT_DESCRIPTOR* ImportDir::rebuild(PeBlock* blocks,
	int nBlocks, Void imgBase, int nImports)
{
	// import directory table
	dataDir[IDE_BOUNDIMP].VirtualAddress = 0;
	dataDir[IDE_BOUNDIMP].Size = 0;	
	IMAGE_IMPORT_DESCRIPTOR* impDesc = NULL;
	for(auto& block : Range(blocks, nBlocks))
	if( block.userData == 0 ) {
		impDesc = imgBase + block.baseRva; 
		break; }
	for(auto& block : Range(blocks, nBlocks)) {
		UserData& ud = *(UserData*)&block.userData;
		int index = ud.dllIndex;
		if(ud.type == Type_NameLst)
			impDesc[index].OriginalFirstThunk = block.baseRva;
		ei(ud.type == Type_FirstThunk)
			this[index].FirstThunk = block.baseRva;
	}
	
	// init import directory table
	memset(&impDesc[nImports], 0, sizeof(*impDesc));
	for(int i = 0; i < nImports; i++) {
		impDesc[i].TimeDateStamp = 0x00000000;
		impDesc[i].ForwarderChain = 0x00000000;
		impDesc[i].FirstThunk = this[i].FirstThunk;
		DWORD* dwIAT = imgBase + impDesc[i].FirstThunk;
		dwIAT[this[i].nImports] = 0;

		DWORD* dwName = imgBase + impDesc[i].OriginalFirstThunk;
		dwName[this[i].nImports] = 0;
		for(int j = 0; j < this[i].nImports; j++)
			dwName[j] = 0x80000000 | this[i].imports[j].Hint();
			
		dwName = imgBase + impDesc[i].FirstThunk;
		dwName[this[i].nImports] = 0;
		for(int j = 0; j < this[i].nImports; j++)
			dwName[j] = 0x80000000 | this[i].imports[j].Hint();
	}

	// import directory names
	for(auto& block : Range(blocks, nBlocks)) {
		UserData& ud = *(UserData*)&block.userData;
		int index = ud.dllIndex;
		if(ud.type == Type_DllName) {
			strcpy(imgBase+block.baseRva, this[index].DllName);
			impDesc[index].Name = block.baseRva; } 
		ei(ud.type == Type_ImpName) {
			int slotIndex = ud.slotIndex;
			(imgBase+impDesc[index].OriginalFirstThunk).
				dword(slotIndex) = block.baseRva;
			(imgBase+impDesc[index].FirstThunk).
				dword(slotIndex) = block.baseRva;
			auto& slot = this[index].imports[slotIndex];
			(imgBase+block.baseRva).word() = slot.Hint();
			strcpy(imgBase+block.baseRva+2, slot.Name());
		}
	}
	return impDesc;
}

bool impCheck(void* ptr, int len, bool free) 
{
	if(!sectChk.check(ptr, len))
		return false;
	if(free == true)
		freeList.mark(ptr, len, Type_ImpDir);
	return true; }
bool impCheck(void* ptr, int len) {
	return impCheck(ptr, len, true); }
bool impCheckStr(Void str) {
	int len = sectChk.checkStr(str);
	if(len < 0) return false;
	freeList.mark(str, len, Type_ImpDir);
	return true; }

int Imports_Load()
{
	int impRva = dataDir[IDE_IMPORT].VirtualAddress;
	if(impRva == 0) return 0;
	IMAGE_IMPORT_DESCRIPTOR* impDesc = rvaToPtr(impRva);
	for(;; impDesc++)
	{
		if(!impCheck(impDesc, sizeof(*impDesc)))
			return 1;
		if( impDesc->Name == 0 )
			break;
		if(!impCheckStr(rvaToPtr(impDesc->Name)))
			return 2;

		// read directory entry
		ImportDir& impDir = xNextAlloc(imports, nImports);
		impDir.DllName = pestrdup(impDesc->Name);	
		impDir.TimeDateStamp = impDesc->TimeDateStamp;	
		impDir.FirstThunk = impDesc->FirstThunk;	
		impDir.imports = 0;
		impDir.nImports = 0;
		
		// read name table
		bool hasOft = impDesc->OriginalFirstThunk;
		DWORD nameRva = !hasOft ? impDesc->FirstThunk :
			impDesc->OriginalFirstThunk;
		DWORD* Names = rvaToPtr(nameRva);
		for(;; Names++)
		{
			if(!impCheck(Names, 4, hasOft))
				return 3;
			if(*Names == 0)
				break;
			IMAGE_IMPORT_BY_NAME* name = rvaToPtr(*Names);
			if(((*Names & 0x80000000) == 0)
			&&((!impCheck(&name->Hint, 2))
			||(!impCheckStr(name->Name))))
				return 4;
				
			ImportSlot& slot = xNextAlloc(
				impDir.imports, impDir.nImports);
			if(*Names & 0x80000000){
				slot.init(NULL, *Names);
			}else{
				slot.init((char*)name->Name, name->Hint);
			}
		}
	}
	return 0;
}

int Import_Find__(char* dllName, char* importName)
{
	ImportDir* newImpDir = NULL;
	for(auto& impDir : Range(imports, nImports))
	if(!stricmp(impDir.DllName, dllName)) {
		if(impDir.FirstThunk == 0)
			newImpDir = &impDir;
		for(int i = 0; i < impDir.nImports; i++)
		if((impDir.imports[i].Name() != NULL)
		&&(!strcmp(impDir.imports[i].Name(), importName))) {
			if(impDir.FirstThunk == 0)
				return 0x80000000;
			return impDir.FirstThunk + i*4; }
	}
	if(newImpDir == NULL)
		return 0x80000001;
	return PTRDIFF(imports,newImpDir)-1;
}

int Import_Find(char* dllName, char* importName)
{
	if(Exports_HasFunc(dllName, importName)) {
		fatal_error("Import_Find: cannot import from self, %s\n", 
			importName); }
	return Import_Find__(dllName, importName);		
}

int Import_Add(char* dllName, char* importName)
{
	if(int exportRva = Exports_HasFunc(dllName, importName))
		return exportRva;
	int result = Import_Find__(dllName, importName);
	if(uint(result) < 0x80000001)	
		return 0;
		
	// allocate import directory
	ImportDir* impDir;
	if(uint(result) != 0x80000001) {
		impDir = Void(imports)-(result+1);
	} else {
		impDir = &xNextAlloc(imports, nImports);
		impDir->DllName = xstrdup(dllName);
		impDir->TimeDateStamp = 0;
		impDir->FirstThunk = 0;
		impDir->imports = NULL;
		impDir->nImports = 0;
	}

	// register import
	auto& import = xNextAlloc(impDir->imports, impDir->nImports);
	import.init(importName, 0);
	return 0;
}

PeSection* Import_UpdateDataDir(bool final)
{
	PeSection* iatSect = NULL;
	int iatBase=0, iatEnd=0;
	for(int i = 0; i < nImports; i++) 
	{
		int baseRva = imports[i].FirstThunk;
		if(baseRva == 0) continue;
		int thunkLen = imports[i].thunkLen();
		PeSection* sect = getSection(baseRva, thunkLen);
		assert(!(final && !sect)); if(!sect) continue;
		if(iatSect == NULL) {
			iatSect = sect;
			iatBase = baseRva;
			iatEnd = baseRva+thunkLen; }
		ei(iatSect == sect) {
			iatBase = min(iatBase, baseRva);
			iatEnd = max(iatEnd, baseRva+thunkLen);  }
		else {
			assert(sect->type() == Type_Data);
		}
	}
	dataDir[IDE_IATABLE].VirtualAddress = iatBase;		
	dataDir[IDE_IATABLE].Size = iatEnd-iatBase;
	dataDir[IDE_BOUNDIMP].VirtualAddress = 0;
	dataDir[IDE_BOUNDIMP].Size = 0;
	return iatSect ? iatSect : peSects;
}

void Imports_Free()
{
	for(auto& import : Range(imports, nImports)) {
		for(auto& slot : Range(import.imports, import.nImports))
		  slot.free();
		free(import.DllName); 
		free(import.imports); }
	free_ref(imports);
	nImports = 0;
}
