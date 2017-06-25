#include <stdshit.h>
#include <win32hlp.h>
#include <findfile.h>
const char progName[] = "test";

xArray<cch*> lib_path32;
xArray<cch*> lib_path64;

void addPath(cstr path)
{
	while(isPathSep(path.getr(-1))) path.slen--;
	bool is64 = (path.slen >= 2) && (RW(path.end()-2) == '46');
	xArray<cch*>& lp = is64 ? lib_path64 : lib_path32;
	for(cch* str : lp) { if(!path.icmp(str)) return; }
	lp.push_back(path.xdup());
}

size_t __stdcall locatePathCb(int err, FindFiles_t& ff)
{
	char* name = ff.getName();
	if((!strcmp(name, "libmsvcrt.a"))
	||(!strcmp(name, "libgcc.a"))
	||(!strcmp(name, "libgcc_s.a"))) {
		addPath(ff.getPath()); }
	return 0;
}

void addPathChk(cch* base, cch* path, int depth)
{
	Cstr tmp = pathCat(base, path);
	if(depth>1) { if(isNeg(getFileAttributes(tmp)))
		return; addPath(tmp); }
	
	switch(depth++) {
	case 0: addPathChk(tmp, "local", depth++); 
	if(0) { case 2: addPathChk(tmp, "64", depth); }
	case 1: addPathChk(tmp, "lib", depth); 
	addPathChk(tmp, "lib32", depth);
	addPathChk(tmp, "lib64", depth); }
}

void print_path(FILE* fp, cch* name, xArray<cch*>& lp)
{
	fprintf(fp, name); name = ";%s"+1;
	for(cch* str : lp) { fprintf(fp,
		name, str); name = ";%s"; }
	fprintf(fp, "\n");
}

int main()
{
	// locate libraries
	char* mingwPath = inputBox(NULL, "exe modifier setup",
		"Please enter path to MinGW", "C:\\MinGW");
	if(mingwPath == NULL) return 0;
	mingwPath = getFullPath(xstr(mingwPath));
	findFiles(mingwPath, 0, 0, locatePathCb);
	if(!lib_path32.len && !lib_path64.len)
		fatalError("mingw libraries not found");
	
	// add other paths of interest
	addPathChk(xstr(mingwPath), "", 0);
	mingwPath = getEnvironmentVariable("PREFIX");
	if(mingwPath) addPathChk(xstr(getFullPath(mingwPath,1)), "", 0);
	mingwPath = getEnvironmentVariable("PROGRAMS");
	if(mingwPath) addPathChk(xstr(getFullPath(mingwPath,1)), "local", 1);
	
	// output the exe_mod.cfg
	FILE* fp = fopen("exe_mod.cfg", "w");
	print_path(fp, "lib_path32=", lib_path32);
	print_path(fp, "lib_path64=", lib_path64);
	fprintf(fp, "def_libs=-lmisc -lmingw32 -lmingwex -lgcc -lmoldname -lmisc "
		"-lmsvcrt -lgdi32 -lcomdlg32 -ladvapi32 -lshell32 -luser32 -lkernel32");
	fprintf(fp, "\nkeep_list=_HookEntryPoint _DllHookCRTStartup@12 .idata$7\n");
	fclose(fp);
}
