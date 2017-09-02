#include "stdafx.h"
#include "exe_mod.h"
#include "miscLib\misc.cpp"
#include "def_file.cpp"

const char progName[] = "exe modifier";
static char** lib_path;
static char** library_list;
static int libraryCount;
char** keep_list;
int keep_count;
bool useHeaderFree;
char exeName[MAX_PATH];
int exePathLen;

SHITSTATIC
void add_library(char* fileName)
{
	xNextAlloc(library_list, libraryCount) 
		= xstrdup(fileName);
}

SHITSTATIC 
void find_library(char* libName)
{
	char filePath[MAX_PATH+32];
	for(int i = 0; lib_path[i]; i++) {
		sprintf(filePath, "%s\\lib%s.a", lib_path[i], libName+2);
		GetFullPathNameA(filePath, MAX_PATH, filePath, NULL);
		if(GetFileAttributesA(filePath) != INVALID_FILE_ATTRIBUTES)
			return add_library(filePath); }
	fatal_error("library not found: %s", libName);
}

SHITSTATIC
void parse_delim_list(char**& str, int& count, const char* delim)
{
	char* pch;
	do {
		pch = strtok(NULL, delim);
		xNextAlloc(str, count) = pch;
	} while(pch);
}

SHITSTATIC 
void readConfig()
{
	// load configuration
	cstr progDir = getProgramDir(); int lineCount;
	char** cfgData = loadText(Cstr(replName(
		progDir, "exe_mod.cfg")), lineCount);
	if(cfgData == NULL)
		fatalError("unable to load config file");

	// parse config
	int listCount = 0;
	xNextAlloc(lib_path, listCount) = progDir;
	for(int i = 0; i < lineCount; i++) {
		char* pch = strtok(cfgData[i], "=");
		if(pch == NULL) continue;
		if(strcmp(pch, "keep_list") == 0)
			parse_delim_list(keep_list, keep_count, " ");
		ei(strcmp(pch, "lib_path32") == 0)
			parse_delim_list(lib_path, listCount, ";");
		ei(strcmp(pch, "def_libs") == 0) {
			while(pch = strtok(NULL, " "))
				find_library(pch);
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

int exe_mod(int argc, char* argv[])
{
	// display ussage
	if( chkInvalidType(argv[1]) ) {
		printf("exe_mod: argument <src exe/dll> invalid\n");
		return 1; }
	if( chkInvalidType(argv[2]) ) {
		printf("exe_mod: argument <dest exe/dll> invalid\n");
		return 1; }
		
	// parse command line
	readConfig();
	int libIndex = libraryCount;
	char** object_list = NULL; int objectCount = 0;
	char** def_list = NULL; int defFileCount = 0;
	bool bindImage = false;
	bool relocCheck = false;
	bool guiMode = false;
	for(int i = 3; i < argc; i++) {
		if(argv[i][0] == '-') {
			if(argv[i][1] == 'r') relocCheck = true;
			ei(argv[i][1] == 'b') bindImage = true;
			ei(argv[i][1] == 'l') find_library(argv[i]);
			ei(!strncmp(argv[i], "-mwindows")) guiMode = true;
			else fatal_error("bad option '%s'\n", argv[i]);
		} ei(!strEicmp(argv[i], ".def")) {
			xNextAlloc(def_list, defFileCount) = argv[i];
		} ei(!strEicmp(argv[i], ".a")) {
			add_library(argv[i]);
		} else {
			xNextAlloc(object_list, objectCount) = argv[i];
		}
	}

	// load pe file
	const char* result = PeFILE::load(argv[1]);
	if(result != NULL) {
		printf("bin_linker: failed to load pe file: %s\n", result);
		return 1; }
	/*DWORD nOldRelocs = PeFILE::nRelocs;
	DWORD* oldRelocs = NULL;
	if((relocCheck == true)&&(nOldRelocs != 0)) {
		oldRelocs = xMalloc(nOldRelocs);
		memcpyX(oldRelocs, PeFILE::relocs, nOldRelocs); } */
	if(guiMode)	PeFILE::subsysGUI();

	// load object files
	dfLink_init();
	for(int i = 0; i < objectCount; i++)
		Linker::object_load(object_list[i]);
	for(int i = 0; i < defFileCount; i++)
		parse_def_file(def_list[i]);
	for(int i = libIndex; i < libraryCount; i++)
		Linker::library_load(library_list[i]);
	dfLink_main();
	for(int i = 0; i < libIndex; i++)
		Linker::library_load(library_list[i]);
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
	if(bindImage == false) { checkSum_file(argv[2]);
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
