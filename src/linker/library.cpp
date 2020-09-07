// library loader
// included by linker.cc
// DeadFish Shitware, 2014

struct LibraryLoad {

	struct Header {
		char fileName[16];
		char timeStamp[12];
		char ownerID[6];
		char groupID[6];
		char fileMode[8];
		char fileSize[10];
		char fileMagic[2]; 
		
		Void getData(void) {
			return fileMagic+2; }
		int getSize(void) {
			return atoi(fileSize); }
		int check(Void fileLimit);
		Header* next(int size) { 
			return getData()+size; }
	};

	const char* fileName;
	Void fileData;
	Void fileLimit;
	
	void load(bool wa);
	__attribute__((noreturn)) void fileBad(cch* msg, Void addr);
	void eofErr(Void addr) {
		fileBad("unexpected end of file", addr); }	
	cstr getName(Header* h, cch* extNames);
};

void LibraryLoad::fileBad(cch* msg, Void addr)
{
	fatal_error("library:%s:%d: %s\n", 
		fileName, addr-fileData, msg);
}

cstr LibraryLoad::getName(Header* h, cch* extNames)
{
	if(h->fileName[0] != '/') { int len = 0; 
		while((len < 16)&&(h->fileName[len]!= '/'))
			len++; return{h->fileName, len};
	} else {
		if(!extNames) fileBad("extended filename without"
			"extended filename section", h);
		cch* begin = extNames+atoi(h->fileName+1);
		if(begin >= extNames) { 
			cch* end = begin; while(end < fileLimit) {
			if(*end == '/') return {begin,end}; end++; }}
		fileBad("extended filename bad", h);
	}
}


int LibraryLoad::Header::check(Void fileLimit)
{
	Void fileData = getData();
	if( fileLimit < fileData )
		return -1;
	if(( fileMagic[0] != 0x60 )
	||( fileMagic[1] != 0x0A ))
		return -1;
	int fileSize = getSize();
	if( fileLimit < (fileData + fileSize))
		return -1;
	return fileSize;
}

void LibraryLoad::load(bool wa)
{
	// check header
	Header* header = fileData+8;
	int fileSize = header->check(fileLimit);
	if((*(WORD*)header->fileName != 0x202F)
	||(fileSize < 4)) fileBad("bad export table", header);
		
	// process exports
	Void exportData = header->getData();
	Void exportLimit = exportData + fileSize;
	int exportCount = bswap32(*exportData.ptr<int>()++);
	char* exportStr = exportData + exportCount*4;

	// check extended names
	char* extNames = NULL;
	header = header->next(fileSize);
	if(((fileSize = header->check(fileLimit)) > 0)
	&&(*(DWORD*)header->fileName == 0x20202F2F)) {
		extNames = header->getData();
		header = header->next(fileSize);
	}
	
	// whole archive
	if(wa) while(1) {
		header = Void((size_t(header)+1) & ~1);
		if(header == fileLimit) break;
		fileSize = header->check(fileLimit);
		if(fileSize < 0) fileBad("bad file", header);
		
		// load the object
		cstr objName = getName(header, extNames);
		object_load(Xstrfmt("%s:%$s", fileName, objName), 
			header->getData(), fileSize);
		header = header->next(fileSize);
	}
		
	int lastExport = -1;
RECHECK_EXPORTS:
	// resolve undefined symbols
	const char* curPos = exportStr;
	for(int i = 0; i < exportCount; i++)
	{
		if(i == lastExport) return;
	
		// check for undefined symbol
		const char* newPos = nullchk(curPos, exportLimit);
		if(!newPos) eofErr(curPos);
		if(needSymbol(curPos)) {
		
		// get object file
		int objIndex = bswap32(exportData.dword(i));
		Header* objHedr = fileData+objIndex;
		int objSize = objHedr->check(fileLimit);
		if(objSize < 0) eofErr(&exportData.dword(i));		
		
		// load the object
		cstr objName = getName(objHedr, extNames);
		object_load(Xstrfmt("%s:%$s", fileName, objName), 
			objHedr->getData(), objSize);	
		lastExport = i;
		}

		curPos = newPos;
	}
	
	if(lastExport >= 0)
		goto RECHECK_EXPORTS;
}

void library_load(const char* fileName, 
	Void fileData, DWORD fileSize, bool wa)
{
	LibraryLoad ll = {fileName, fileData,
		fileData+fileSize}; ll.load(wa);
}
