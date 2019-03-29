#include "stdafx.h"
#include "exe_mod.h"
#include "sha1.h"

struct ExmFile
{
	// Exm file structures
	struct ExmHead { char magic[8];
		DWORD nFiles; DWORD nSets; };
	struct ExmData { DWORD size; BYTE data[]; };
	struct ExmArg { WORD wVal; WORD sLen; char str[]; };
	
	// Exm memory structures
	struct FileInfo : xarray<byte> { 
		Sha1 hash; int fileId; void doHash() 
		{ hash.calc(data, size); }};
	struct ArgInfo { int fileId; char* arg; };
	struct SetInfo : xArray<ArgInfo> { cch* name; };
	xArray<FileInfo> fileList;
	xArray<SetInfo> setList;
	
	// write access
	SetInfo* findSet(cch* setName);
	SetInfo& addSet(cch* setName);
	int addFile(void* data, DWORD size);	
	
	
	
	
	bool save(cch* fileName);
	bool load(byte* data, int size);
	
private:
	
	void writeArg(FILE* fp, int wVal, cch* sVal);
	ExmArg* readArg(xRngPtr<byte>& curPos);
};

void ExmFileWrite(int argc, char* argv[])
{
	// get the set name
	char* setName = getName(argv[1]).rchr(':');
	if(setName) WRI(setName, 0);
	else setName = (char*)"";

	// load existing file
	ExmFile exmFile;
	if(argv[1][0] == '+') { argv[1]++;
		auto file = loadFile(argv[1]);
		if((file)&&(!exmFile.load(file.data, file.size)))
			fatal_error("bad emx file: %s", argv[1]);
	}
	
	// read the files
	auto& set = exmFile.addSet(setName);
	for(int i = 2; i < argc; i++) {
		int dataId = -1;
		if(argv[i][0] != '-') {
			auto data = loadFile(argv[i]);
			if(!data) load_error("file", argv[i]);
			dataId = exmFile.addFile(data, data.size); }
		set.push_back(dataId, argv[i]);
	}
	
	// write the file
	if(!exmFile.save(argv[1])) fatal_error(
		"write error: %s\n", argv[1]);
	exit(0);
}

void ExmFileRead(FileOrMem& fileRef, 
	cch* setName, Delegate<void,FileOrMem> cb)
{
	// load exm file
	ExmFile exm; int size = fileRef.open();
	if(size < 0) load_error("exm file", fileRef.name);
	if(!exm.load((byte*)fileRef.data, size))
		fatal_error("bad exm file: %s\n", fileRef.name);

	// locate specified set
	printf("set: %s\n", setName);
	
	
	
	if(!setName) setName = "";
	auto* set = exm.findSet(setName);
	
	
	
	if(!set) fatal_error("exm set not found");
	
	// iterate over arguments
	for(auto& arg : *set) {
		xarray<byte> data = {};
		if(arg.fileId >= 0) data = 
			exm.fileList[arg.fileId];
		cb({arg.arg, data.data, data.size});
	}
}
ExmFile::SetInfo* ExmFile::findSet(cch* setName)
{
	for(auto& set : setList)
		if(!stricmp(set.name, setName))
			return &set; return NULL;
}

ExmFile::SetInfo& ExmFile::addSet(cch* setName)
{
	auto* set = findSet(setName);
	if(set) { set->Clear(); }
	else { set = &setList.push_back();
		set->name = setName; }
	return *set;
}

int ExmFile::addFile(void* data, DWORD size)
{
	// locate existing item
	Sha1 hash; hash.calc(data, size);
	for(auto& file : fileList) {
		if(file.hash == hash)
			return &file-fileList.data;	}
	
	// create new item
	auto& file = fileList.xnxalloc();
	file.init((byte*)data, size); 
	file.hash.calc(data, size);
	return &file-fileList.data;
}

void ExmFile::writeArg(
	FILE* fp, int wVal, cch* sVal)
{
	ExmArg arg = {wVal, strlen(sVal)};
	xfwrite(arg, fp);
	xfwrite(sVal, arg.sLen+1, fp);
}

ExmFile::ExmArg* ExmFile::readArg(xRngPtr<byte>& curPos)
{
	ExmArg* head = curPos.get(sizeof(ExmArg));
	if(!head || !curPos.get(head->sLen+1)
	|| head->str[head->sLen]) return NULL;
	return head;
}

bool ExmFile::save(cch* fileName)
{
	// assign fileIds
	int nFiles = 0;
	for(auto& file : fileList) file.fileId = 0;
	for(auto& set : setList) for(auto& arg : set) 
	if(arg.fileId >= 0)	fileList[arg.fileId].fileId = 1;
	for(auto& file : fileList) if(file.fileId)
		file.fileId = ++nFiles; 
		
	// write header
	FILE* fp = xfopen(fileName, "wb");
	if(!fp) return false;
	ExmHead head = { "DFEXMOD0", nFiles, setList.len };
	xfwrite(head, fp);
	
	// write files
	for(auto& file : fileList) {
		if(file.fileId) { xfwrite(file.size, fp);
		xfwrite(file.data, file.size, fp); }
	}
	
	// write sets
	for(auto& set : setList) { 
	writeArg(fp, set.len, set.name);
	for(auto& arg : set) {
		WORD fileId = (arg.fileId < 0) ?
			0 : fileList[arg.fileId].fileId;
		writeArg(fp, fileId, arg.arg);
	}}
	
	return true;
}

bool ExmFile::load(byte* data, int size)
{
	// read the header
	xRngPtr<byte> curPos = {data, size};
	ExmHead* headr = curPos.get(sizeof(ExmHead));
	if(headr == NULL) return false;
	
	// read the files
	int nFiles = headr->nFiles;
	while(--nFiles >= 0) {
		byte* data; ExmData* head;
		if(!(head = curPos.get(sizeof(ExmData)))||
		(!(data = curPos.get(head->size)))) return 1;
		auto& file = fileList.xnxalloc();
		file.init(data, head->size); 
		file.hash.calc(data, head->size);
	}
	
	// read the sets
	int nSets = headr->nSets;
	while(--nSets >= 0) { 
		ExmArg* head = readArg(curPos);
		if(head == NULL) return false;
		auto& set = setList.push_back();
		set.name = head->str;
		int nArgs = head->wVal;set.len;
		while(--nArgs >= 0) {
			ExmArg* head = readArg(curPos);
			if((head == NULL)||(head->wVal
			> headr->nFiles)) return false;
			set.push_back(head->wVal-1, head->str);
		}
	}
	
	return true;
}

char* grpCat(cch* str, cch* grp)
{
	cch* atPos = strrchr(str, '@');
	if(atPos == NULL) { fatal_error(
		"'@' not present in argument: \"%s\"", str); }
	cstr tmp = getName2(grp);
	return xstrfmt("%v%v~%s", cstr(
		str, atPos), tmp, tmp.end());
}

char* verCat(cch* str, cch* ver)
{
	cch* tiPos = strrchr(str, '~');
	if(tiPos == NULL) { fatal_error(
		"'~' not present in argument: \"%s\"", str); }
	return xstrfmt("%v;%s%s", cstr(
		str, tiPos), ver, tiPos+1);
}

void doCall(cch* cmdLine)
{
	STARTUPINFOA si = {}; PROCESS_INFORMATION pi;
	createProcess(xstr(getModuleFileName(0)),
		cmdLine, 0, 0, 0, 0, 0, 0, &si, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);	
	CloseHandle(pi.hProcess);
}


void ExmFileCall2(int argc, char* argv[])
{
	struct ArgInfo {
		char* str; ExmFile exm; };
	xarray<ArgInfo> args = {};
	xarray<cch*> ver = {};

	// read arguments
	for(int i = 3; i < argc; i++) {
		auto& arg = args.push_back(argv[i]);
		if(!strEicmp(argv[i], ".exm")) 
		{
			// load the exm file
			auto file = loadFile(argv[i]);
			if(!file) load_error("exm file", argv[i]);
			if(!arg.exm.load(file.data, file.size))
			fatal_error("bad exm file: %s\n", argv[i]);	
			
			// add sets to set list
			for(auto& set : arg.exm.setList) {
			for(cch* sver : ver) {
				if(!stricmp(sver, set.name)) goto FOUND_VER; }
			ver.push_back(set.name); FOUND_VER:; }
		}
	}
	
	// clobber unamed set
	if(ver.size > 1) for(cch*& sver : ver) {
	if(*sver == '\0') { sver = ver.back(); 
		ver.pop_back(); break; }}

	// iterate verions
	for(int i = 0; i < ver.len; i++) {
		bstr cmd = "exe_mod.exe";
		cmd.argcatf(verCat(argv[1], ver[i]));
		cmd.argcatf(verCat(argv[2], ver[i]));
		for(auto& arg : args) {
			if(arg.exm.findSet(ver[i])) 
				cmd.fmtcat(" %z:%s", arg.str, ver[i]);
			ei(!arg.exm.setList || arg.exm.findSet(""))
				cmd.argcat(arg.str);
		}
		
		printf("%s\n", cmd);
		
		doCall(cmd);
	}

	exit(0);
}

void ExmFileCall(char mode, int argc, char* argv[])
{
	if(mode == '~') { ExmFileCall2(argc, argv); }
	struct ArgInfo {
		char* str; int groupId; };
	xarray<ArgInfo> args = {};
	xarray<char*> grp = {};
	
	// read arguments
	for(int i = 3; i < argc; i++) {
		cstr name = getName2(argv[i]);
		int groupId = -1;
		if(!stricmp(name.end(), ".exm")) {
		if(!(name = cstr_split(name, ';'))) fatal_error(
		"exm file name lacks group: \"%s\"", argv[i]);
		while(++groupId < grp.len) { if(!name
			.cmp(grp[groupId])) goto GRP_FOUND; }
		grp.push_back(name.xdup()); GRP_FOUND:; }
		args.push_back(argv[i], groupId);
	}

	// iterate groups
	for(int i = 0; i < grp.len; i++) {
		bstr cmd = "exe_mod.exe";
		cmd.argcatf(grpCat(argv[1], grp[i]));
		cmd.argcatf(grpCat(argv[2], grp[i]));
		for(auto& arg : args) if((arg.groupId < 0)
		||(arg.groupId == i)) cmd.argcat(arg.str);
		doCall(cmd);
	}
	
	exit(0);
}
