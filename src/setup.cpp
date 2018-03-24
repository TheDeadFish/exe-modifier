#include <stdshit.h>
#include <win32hlp.h>
#include <findfile.h>
const char progName[] = "test";

xArray<cch*> lib_path32;
xArray<cch*> lib_path64;

bool addPath(cstr fName, int mode)
{
	// determin architecture
	xArray<byte> file = loadFile(fName);
	if(!file || !file.chk<u32>(72)|| strncmp
	((char*)file.data, "!<arch>\n")) return false;
	byte* objPtr = file.getp(bswap32
		(file.getr<u32>(72)),62);
	if(!objPtr) return false;
	bool is64 = RW(objPtr+60) == 0x8664;
	if(!(is64 ^ mode)) return false;
	
	// add path to list
	cstr path = getPath(fName);
	while(isPathSep(path.getr(-1))) path.slen--;
	xArray<cch*>& lp = is64 ? lib_path64 : lib_path32;
	for(cch* str : lp) { if(!path.icmp(str)) goto L1; }
	lp.push_back(path.xdup()); L1: return true;
}

size_t __stdcall locatePathCb(int err, FindFiles_t& ff)
{
	char* name = ff.getName();
	if((!strcmp(name, "libmsvcrt.a"))
	||(!strcmp(name, "libgcc.a"))
	||(!strcmp(name, "libgcc_s.a"))) {
		addPath(ff.fName, size_t(ff.ctx)); }
	return 0;
}

size_t __stdcall locatePathCb2(int err, FindFiles_t& ff)
{
	cstr ext = getExt(ff.getName());
	if(!ext.icmp(".a")) {
		addPath(ff.fName, size_t(ff.ctx)); }
	return 0;
}

void print_path(FILE* fp, cch* name, xArray<cch*>& lp)
{
	fprintf(fp, name); name = ";%s"+1;
	for(cch* str : lp) { fprintf(fp,
		name, str); name = ";%s"; }
	fprintf(fp, "\n");
}

void find_paths(cch* mingwPath, int x86Mode)
{
	// locate libraries
	mingwPath = getFullPath(xstr(mingwPath));
	findFiles(mingwPath, 0, (void*)x86Mode, locatePathCb);
	if(!lib_path32.len && !lib_path64.len)
		fatalError("mingw libraries not found");	
}

int main()
{
	// select the paths
	cch* x86Path = selectFolder(NULL, 
		"select MinGW x86 path", 0);
	cch* x64Path = selectFolder(NULL, 
		"select MinGW x64 path", x86Path);
		
	if(!x86Path && !x64Path) return 0;
	find_paths(x86Path, 1);
	find_paths(x64Path, 0);
		
	// add other paths of interest
	cch* mingwPath = getEnvironmentVariable("PREFIX");
	if(mingwPath) findFiles(xstr(getFullPathF(
		mingwPath)), 0, (void*)-1, locatePathCb2);
	mingwPath = getEnvironmentVariable("PROGRAMS");
	if(mingwPath) findFiles(xstr(fullNameCat(
		mingwPath,"local")), 0, (void*)-1, locatePathCb2);	
	
	// output the exe_mod.cfg
	FILE* fp = fopen("exe_mod.cfg", "w");
	print_path(fp, "lib_path32=", lib_path32);
	print_path(fp, "lib_path64=", lib_path64);
	fprintf(fp, "def_libs=-lmisc -lmingw32 -lmingwex -lgcc -lmoldname -lmisc "
		"-lmsvcrt -lgdi32 -lcomdlg32 -ladvapi32 -lshell32 -luser32 -lkernel32");
	fprintf(fp, "\nkeep_list=.idata$7\n");
	fclose(fp);
}
