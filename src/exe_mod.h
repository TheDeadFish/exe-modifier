#ifndef _EXE_MODIF_H_
#define _EXE_MODIF_H_
#include "peFile/peFile_.h"
#include "linker/linker.h"

// error handling
static FATALFUNC
void fatal_error(const char*fmt,...) {
	va_list args; va_start (args, fmt);
	vfprintf(stderr, fmt,args);
	exit(1); va_end (args); }
#define FATAL_ERROR(str, arg, fileName) \
	fatal_error(str ", \"%s\"\n", arg, fileName);
static FATALFUNC
void load_error(const char* type, const char* fileName) {
	FATAL_ERROR("%s: open failed", type, fileName); }
static FATALFUNC
void file_corrupt(const char* type, const char* fileName) {
	FATAL_ERROR("%s: file corrupt", type, fileName); }
static FATALFUNC
void file_bad(const char* type, const char* fileName) {
	FATAL_ERROR("%s: unsupported file data", type, fileName); }
static FATALFUNC
void recurse_error(const char* type) {
	fatal_error("%s: infinite recursion error", type); }
	
#define error_msg(fmt, ...) \
	fprintf(stderr, fmt, __VA_ARGS__);

// dfLinker functions
int exe_mod(int argc, char* argv[]);
void dfLink_init();
void dfLink_main();
Linker::Symbol* dfLink_entryPoint();

// exmod file interface
struct FileOrMem { char* name;
	void* data; DWORD size_;
	
	FileOrMem() = default;
	FileOrMem(char* nm) : FileOrMem(nm, 0, 0) {}
	FileOrMem(char* a, void* b, DWORD c) :
		name(a), data(b), size_(c) {}
	
	int open(int extra = 0); void free(void);
	//DWORD size() { return size_ & INT_MAX; }
};

void WINAPI ExmFileWrite(int argc, char* argv[]);
void WINAPI ExmFileRead(FileOrMem& fileRef, 
	cch* setName, Delegate<void,FileOrMem> cb);
void WINAPI ExmFileCall(char mode, int argc, char* argv[]);

// architecture dependant strings
struct ArchStr { cch* libMisc; cch* libPath;
cch* rawEntryPoint; cch* hookEntryPoint;
cch* dllMainStartup; cch* dllHookStartup;
cch* sectionStart; cch* sectionStop;
}; extern const ArchStr* archStr;

static bool x64Mode() { return PeFILE::peFile.PE64; }
static int ptrSize() { return x64Mode() ? 64 : 32; }

// debugging
void dump_sections(FILE* fp);
void dump_symbols(FILE* fp);

// options
extern bool g_noSymFix;



#endif
