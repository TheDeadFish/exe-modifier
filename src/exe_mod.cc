#include "stdafx.h"
#include "exe_mod.h"
#include "miscLib\misc.cpp"
#include "def_file.cpp"

const char progName[] = "exe modifier";

int FileOrMem::open(int extra)
{
	if(data) return size_;
	auto file = loadFile(name, extra);
	data = file.data;
	size_ = file.size | INT_MIN;
	return file.size;
}

void FileOrMem::free(void)
{
	if(size_ & INT_MIN) free_ref(data);
}

void library_load(FileOrMem& fileRef)
{
	int size = fileRef.open();
	if(size < 0) { load_error(
		"library", fileRef.name); }
	Linker::library_load(fileRef.name,
		fileRef.data, size);
	fileRef.free();
}

void object_load(FileOrMem& fileRef)
{
	int size = fileRef.open();
	if(size < 0) { load_error(
		"object", fileRef.name); }
	Linker::object_load(fileRef.name,
		fileRef.data, size);
	fileRef.free();
}

void defFile_load(FileOrMem& fileRef)
{
	int size = fileRef.open(1);
	if(size < 0) load_error(
		"def file", fileRef.name);
	PB(fileRef.data)[size] = 0; 
	parse_def_file(fileRef.name, fileRef.data);
	fileRef.free();
}

struct Arguments
{
	bool bindImage, guiMode;
	xArray<cch*> libPaths;
	xArray<FileOrMem> libs;
	xArray<FileOrMem> objs;
	xArray<FileOrMem> defs;
	
	Arguments() { ZINIT; }
	
	
	void find_library(char* libName);
	
	void next(FileOrMem fileRef);
};

void Arguments::find_library(char* libName)
{
	for(cch* libPath : libPaths) {
		char* name = xstrfmt("%j%:lib%s.a", libPath, libName+2);
		if(!isNeg(getFileAttributes(name))) { libs
			.push_back(name); return;  } free(name); }
	fatal_error("library not found: %s", libName);
}


SHITSTATIC 
void readConfig(Arguments& args)
{
	// load configuration
	cstr progDir = getProgramDir(); int lineCount;
	char** cfgData = loadText(Cstr(replName(
		progDir, "exe_mod.cfg")), lineCount);
	if(cfgData == NULL)
		fatalError("unable to load config file");

	// parse config
	args.libPaths.push_back(progDir); 
	for(int i = 0; i < lineCount; i++) {
		char* pch = strtok(cfgData[i], "=");
		if(pch == NULL) continue;
		
		
		
		if(strcmp(pch, "keep_list") == 0) {
			while(pch = strtok(NULL, " "))
				Linker::keepSymbol(pch);
		}
	
		ei(strcmp(pch, "lib_path32") == 0) {
			while(pch = strtok(NULL, ";"))
				args.libPaths.push_back(pch);
		}
		
		ei(strcmp(pch, "def_libs") == 0) {
			while(pch = strtok(NULL, " "))
				args.find_library(pch);
		}
	}
}

SHITSTATIC
bool chkInvalidType(char* fileName)
{
	return strEicmp(fileName, ".exe") 
		&& strEicmp(fileName, ".dll")
		&& strEicmp(fileName, ".sys")
		&& strEicmp(fileName, ".cpl");
}

void checkSum_file(char* fileName)
{
	HANDLE FileHandle = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) { CHECKSUM_FAIL:
		fatal_error("bin_linker: checksum failed\n"); }
	HANDLE MappingHandle = CreateFileMapping(FileHandle,
			NULL, PAGE_READWRITE, 0, 0, NULL);
	if(MappingHandle == NULL) goto CHECKSUM_FAIL;
	LPVOID BaseAddress = MapViewOfFile(MappingHandle,
		FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	CloseHandle(MappingHandle);	
	if(BaseAddress == NULL) goto CHECKSUM_FAIL;
    DWORD FileLength = GetFileSize( FileHandle, NULL );
	DWORD HeaderSum, CheckSum;
	auto* hdr = CheckSumMappedFile(BaseAddress,
		FileLength, &HeaderSum, &CheckSum);
	hdr->OptionalHeader.CheckSum = CheckSum;
	UnmapViewOfFile(BaseAddress);
    CloseHandle( FileHandle );
}



void Arguments::next(FileOrMem fileRef)
{
	char* arg = fileRef.name;
	if(arg[0] == '-') {
		if(arg[1] == 'b') { bindImage = true; }
		ei(arg[1] == 'l') { find_library(arg); }
		ei(!strncmp(arg, "-mwindows")) guiMode = true;
		else fatal_error("bad option '%s'\n", arg);
	} else {
		char* name = getName(arg);
		char* setName = strrchr(name, ':');
		if(setName) WRI(setName, 0);

		if(!strEicmp(arg, ".exm")) {
			ExmFileRead(fileRef, setName,
			MakeDelegate(this, Arguments::next));
		}ei(!strEicmp(arg, ".def"))
			defs.push_back(fileRef);
		ei(!strEicmp(arg, ".a"))
			libs.push_back(fileRef);
		else objs.push_back(fileRef);
		
	}
}

int exe_mod(int argc, char* argv[])
{
	// check for exm file
	if(getName(argv[1]).istr(".exm"))
		ExmFileWrite(argc, argv);
		
	// check for exm call
	{char ch = getName2(argv[1]).getr(-1);
	if(is_one_of(ch, '@', '~')) {
		ExmFileCall(ch, argc, argv); }}

	// display ussage
	if( chkInvalidType(argv[1]) ) {
		printf("exe_mod: argument <src exe/dll> invalid\n");
		return 1; }
	if( chkInvalidType(argv[2]) ) {
		printf("exe_mod: argument <dest exe/dll> invalid\n");
		return 1; }
		
	// parse command line
	Arguments args; readConfig(args);
	int libIndex = args.libs.len;
	for(int i = 3; i < argc; i++) {
		args.next(argv[i]); }

	// load pe file
	const char* result = PeFILE::load(argv[1]);
	if(result != NULL) {
		printf("bin_linker: failed to load pe file: %s\n", result);
		return 1; }
	if(args.guiMode) PeFILE::subsysGUI();

	// load object files
	dfLink_init();
	for(auto& obj : args.objs) object_load(obj);
	for(auto& def : args.defs) defFile_load(def);
	for(auto& lib : args.libs.right(libIndex))
		library_load(lib);
	dfLink_main();
	for(auto& lib : args.libs.left(libIndex))
		library_load(lib);
	Linker::exports_symbfix();
	Linker::gc_sections();
	Linker::imports_parse();

	// allocate sections
	PeBlock* blocks = xCalloc(Linker::nSections);
	int blockCount = 0;
	for(int i = 0; i < Linker::nSections; i++) {
		if( Linker::sections[i].type >= 4 ) continue;
		
		static const byte types[] = {
			PeSecTyp::Data, PeSecTyp::Bss, 
			PeSecTyp::RData, PeSecTyp::Text };
		blocks[blockCount].type = types[Linker::sections[i].type];
		
		blocks[blockCount].align = Linker::sections[i].align;
		blocks[blockCount].length = Linker::sections[i].length;
		blocks[blockCount++].lnSect = i; }
	PeFILE::allocBlocks(blocks, blockCount);
	
	// initialize sections
	for(int i = 0; i < blockCount; i++) {
		auto& sect = Linker::sections[blocks[i].lnSect];
		sect.baseRva = blocks[i].baseRva;
		Void basePtr = PeFILE::rvaToPtr(sect.baseRva);
		memcpy(basePtr, sect.rawData, sect.length);
		free(sect.rawData); sect.rawData = basePtr; }
	Linker::imports_resolve();
	Linker::exports_resolve();
	Linker::relocs_fixup();

	// write output file
	int entryPoint = dfLink_entryPoint();
	if(entryPoint >= 0)
		PeFILE::entryPoint() = Linker::symbolRva(entryPoint);
	result = PeFILE::save(argv[2]);
	if(result != NULL) {
		printf("bin_linker: failed to save pe file: %s\n", result);
		return 1; }
	
	// bind/checksum pe file	
	if(args.bindImage == false) { checkSum_file(argv[2]);
	} ei(!BindImage(argv[2], NULL, NULL)) {
		fatal_error("bin_linker: binding failed\n"); return 1; }
		
	/* peform checks
	if(relocCheck == true) 
	{
		result = PeFILE::load(argv[2]);
		if(result != NULL) {
			printf("bin_linker: failed to reload pe file: %s\n", result);
		return 1; }
		PeFILE::Relocs_Report(oldRelocs, nOldRelocs);
	}*/
}
