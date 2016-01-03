// library loader
// included by linker.cc
// DeadFish Shitware, 2014

void library_load(const char* fileName,
	Void fileData, int fileSize)
{
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
		int check(Void fileLimit) {
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
	};

	// check header
	Void fileLimit = fileData+fileSize;
	Header* header = fileData+8;
	int exportSize = header->check(fileLimit);
	if(exportSize < 0)
		file_corrupt("library", fileName);
	if((exportSize < 4)
	||( !strScmp(header->fileName, "/               ")))
		file_bad("library", fileName);
		
	// process exports
	Void exportData = header->getData();
	Void exportLimit = exportData + exportSize;
	int exportCount = bswap32(*exportData.ptr<int>()++);
	char* exportStr = exportData + exportCount*4;
	char* objectRef = xmalloc(exportCount);
	int lockupCheck = 0;
	
	// prepare name
	char libFileName[MAX_PATH+17] = {0};
	char* libNamePos = libFileName + 
		strNcpy(libFileName, fileName, MAX_PATH);
	*libNamePos++ = ':';
	
RECHECK_EXPORTS:
	// resolve undefined symbols
	const char* curPos = exportStr;
	memset(objectRef, 0, exportCount);
	for(int i = 0; i < exportCount; i++)
	{
		const char* newPos = nullchk(curPos, exportLimit);
		if(newPos == NULL)
			file_corrupt("library", fileName);
		int symbIndex = findSymbol(curPos);
		if(( symbIndex >= 0 )
		&&( symbols[symbIndex].section == Type_Undefined ))
			objectRef[i] = 1;
		curPos = newPos;
	}
	
	// load objects
	bool recheck = false;
	for(int i = 0; i < exportCount; i++)
	  if(objectRef[i] != 0)
	{
		int objIndex = bswap32(exportData.dword(i));
		Header* objHedr = fileData+objIndex;
		int objSize = objHedr->check(fileLimit);
		if(objSize < 0)
			file_corrupt("library", fileName);
			
		memcpy(libNamePos, objHedr->fileName, 16);
		char* spacePos = strchr(libNamePos, ' ');
		if(spacePos != NULL) *spacePos = '\0';
		object_load(libFileName, objHedr->getData(), objSize);
		recheck = true;
	}
	if(recheck == true) {
		if(lockupCheck++ > 20)
			recurse_error("library");
		goto RECHECK_EXPORTS; 
	}
}

void library_load(const char* fileName)
{
	FILE* fp = xfopen(fileName, "rb");
	if(fp == NULL) load_error("library", fileName);
	int fileSize = fsize(fp);
	Void fileData = xmalloc(fileSize);
	xfread((char*)fileData, fileSize, fp);
	fclose(fp);
	library_load(fileName, fileData, fileSize);
}
