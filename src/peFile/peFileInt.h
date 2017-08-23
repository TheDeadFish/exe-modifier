
	struct FreeList;
	void Import_Blocks(FreeList& freeList,
		PeBlock*& blocks, int nBlocks);
	
	PeFile_ peFile;
		
	// Internal structures
	FreeList freeList;
	struct RangeChk {
		Void basePtr; Void endPtr;
		RangeChk() {}
		RangeChk(Void base, Void end) {
			basePtr = base; endPtr = end; }
		bool check(Void ptr, int length = 4);
		int checkStr(Void str); };
	RangeChk sectChk, rsrcChk;


	// Complex helpers
	int Imports_Load(); int Imports_Save();
	const char* Exports_Load();
	//PeSection* addSect(const char* name, int size, bool isData);

	// Simple helpers
	Void rvaToPtr(int rva); 
	DWORD ptrToRva(void* ptr);
	bool checkRva(int rva, int length = 4);
	bool checkPtr(Void ptr, int length = 4);
	char* pestrdup(int rva);


	// Data directories shorthand
	enum {
		IDE_EXPORT 		= IMAGE_DIRECTORY_ENTRY_EXPORT,
		IDE_IMPORT 		= IMAGE_DIRECTORY_ENTRY_IMPORT,
		IDE_RESOURCE 	= IMAGE_DIRECTORY_ENTRY_RESOURCE,
		IDE_EXCEPTION 	= IMAGE_DIRECTORY_ENTRY_EXCEPTION,
		IDE_SECURITY 	= IMAGE_DIRECTORY_ENTRY_SECURITY,
		IDE_BASERELOC 	= IMAGE_DIRECTORY_ENTRY_BASERELOC,
		IDE_DEBUG 		= IMAGE_DIRECTORY_ENTRY_DEBUG,
		IDE_ARCH 		= IMAGE_DIRECTORY_ENTRY_ARCHITECTURE,
		IDE_GLOBL	 	= IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
		IDE_TLS 		= IMAGE_DIRECTORY_ENTRY_TLS,
		IDE_CONFIG 		= IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
		IDE_BOUNDIMP	= IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
		IDE_IATABLE		= IMAGE_DIRECTORY_ENTRY_IAT,
		IDE_DELAYIMP	= IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
		IDE_COM_DESC	= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR	
	};
