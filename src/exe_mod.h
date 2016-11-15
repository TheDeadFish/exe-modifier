#ifndef _EXE_MODIF_H_
#define _EXE_MODIF_H_
#include "peFile/peFile.h"
#include "linker/linker.h"

// global config
extern char** keep_list;
extern int keep_count;
extern bool useHeaderFree;

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
int dfLink_entryPoint();

#endif
