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
	char* setName = strrchr(argv[2], ':');
	if(setName) *setName = 0;
	else setName = (char*)"";

	// load existing file
	ExmFile exmFile;
	if(argv[2][0] == '+') {
		auto file = loadFile(argv[2]+1);
		if((file)&&(!exmFile.load(file.data, file.size)))
			fatal_error("bad emx file: %s", argv[2]+1);
	}
	
	// read the files
	auto& set = exmFile.addSet(setName);
	for(int i = 3; i < argc; i++) {
		int dataId = -1;
		if(argv[i][0] != '-') {
			auto data = loadFile(argv[i]);
			if(!data) load_error("file", argv[i]);
			dataId = exmFile.addFile(data, data.size); }
		set.push_back(dataId, argv[i]);
	}
	
	// write the file
	if(!exmFile.save(argv[2])) fatal_error(
		"write error: %s\n", argv[2]);
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
	if(!set) { set = &setList.push_back();
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
		file.fileId = nFiles++; 
		
	// write header
	FILE* fp = xfopen(fileName, "wb");
	if(!fp) return false;
	ExmHead head = { "DFEXMOD0", nFiles, setList.len };
	xfwrite(head, fp);
	
	// write files
	for(auto& file : fileList) {
		xfwrite(file.size, fp);
		xfwrite(file.data, file.size, fp);
	}
	
	// write sets
	for(auto& set : setList) { 
	writeArg(fp, set.len, set.name);
	for(auto& arg : set) {
		WORD fileId = (arg.fileId < 0) ?
			0 : fileList[arg.fileId].fileId+1;
		writeArg(fp, fileId, arg.arg);
	}}
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
