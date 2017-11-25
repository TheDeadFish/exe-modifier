#include <stdshit.h>
const char progName[] = "stub make";

cch* stub_format = ".text\n .globl %s\n .def "
	"%s; .scl 2; .type 32; .endef\n %s: \n";
cch* ins_jmp_imp32 = "jmp *__imp_%s\n";
cch* ins_jmp_imp64 = "jmp *__imp_%s(%%rip)\n";
cch* ins_ret = "ret\n";
	
void make_object(cch* fName, cch* fnName, 
	cch* insFmt, int bits)
{
	// start as
	cch* cmd = xstrfmt("as -o obj%d\\%s.o --%d",
		bits, fName, bits);
	FILE* fp = popen(cmd, "w");
	
	// pipe out assembly
	if(bits==32) fnName = xstrfmt("_%s", fnName);
	fprintf(fp, stub_format, fnName,fnName,fnName);
	fprintf(fp, insFmt, fnName);
	pclose(fp);
}

void mame_library(int bits)
{
	cch* ins_jmp_imp = (bits==64) ? ins_jmp_imp64 : ins_jmp_imp32;
	make_object("atexit", "atexit", ins_jmp_imp, bits);
	make_object("snprintf", "snprintf", ins_jmp_imp, bits);
	make_object("vsnprintf", "vsnprintf", ins_jmp_imp, bits);
	make_object("assert", "_assert", ins_jmp_imp, bits);
	make_object("matherr", "__mingw_raise_matherr", ins_ret, bits);
	sysfmt("ar -rcs ..\\..\\bin\\libmisc%d.a obj%d\\*.o", bits, bits);

}

int main()
{		
	mame_library(32);
	mame_library(64);
}
