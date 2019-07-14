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
	};

	const char* fileName;
	Void fileData;
	Void fileLimit;
	
	void load(void);
	__attribute__((noreturn)) void fileBad(cch* msg, Void addr);
	void eofErr(Void addr) {
		fileBad("unexpected end of file", addr); }	
	cstr getName(Header* h, cch* extNames);
};

void LibraryLoad::fileBad(cch* msg, Void addr)
{
	fatal_error("library:%s:%d: %s\n", addr-fileData, msg);
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

void LibraryLoad::load()
{
	// check header
	Header* header = fileData+8;
	int exportSize = header->check(fileLimit);
	if((*(WORD*)header->fileName != 0x202F)
	||(exportSize < 4)) fileBad("bad export table", header);
		
	// process exports
	Void exportData = header->getData();
	Void exportLimit = exportData + exportSize;
	int exportCount = bswap32(*exportData.ptr<int>()++);
	char* exportStr = exportData + exportCount*4;
	
	// check extended names
	char* extNames = NULL;
	{ Header* extNames_ = header->getData()+exportSize;
	if((extNames_->check(fileLimit) > 0)
	&&(*(DWORD*)extNames_->fileName == 0x20202F2F))
		extNames = extNames_->getData(); }
		
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
	Void fileData, DWORD fileSize)
{
	LibraryLoad ll = {fileName, fileData,
		fileData+fileSize}; ll.load();
}
