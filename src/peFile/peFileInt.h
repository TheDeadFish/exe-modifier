
	struct FreeList;
	void Import_Blocks(FreeList& freeList,
		PeBlock*& blocks, int nBlocks);
	
	// pe file basic information
	struct PeSection : public IMAGE_SECTION_HEADER
	{
		const char* name(void);
		DWORD& vSize(void);
		DWORD& baseRva(void);
		DWORD endRva(void);
		DWORD endPage(void);
		Void& basePtr(void);
		Void endPtr(void);
		int type(void);
		int remain(void);
		int extent(void);
	} *peSects;
	WORD& nSections(void);
	DWORD& imageBase(void);
	
	// resizing information
	PeSection* extndSection;
	PeSection* rsrcSection;
	PeSection* relocSection;	
	
	// internal data
	Void imageData; DWORD imageSize;
	Void fileExtra;	DWORD extraSize;
	IMAGE_FILE_HEADER* fileHeadr;
	IMAGE_OPTIONAL_HEADER* optHeadr;
	IMAGE_DATA_DIRECTORY* dataDir;

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
	DWORD calcExtent(
		Void buff, DWORD length);
	int peRealloc(DWORD delta);
	bool checkHeadr(void);
	int Relocs_Load();	int Relocs_Save();
	int Imports_Load(); int Imports_Save();
	const char* Exports_Load();

	bool rebaseRsrc( int offset, 
		IMAGE_RESOURCE_DIRECTORY* rsrcDir );
	void peResize(PeSection* sect, int newSize);
	PeSection* addSect(const char* name, int size, bool isData);
	int headerSize(int& offset);

	// Simple helpers
	Void rvaToPtr(int rva); 
	DWORD ptrToRva(void* ptr);
	bool checkRva(int rva, int length = 4);
	bool checkPtr(Void ptr, int length = 4);
	char* pestrdup(int rva);
	int sectIdx(PeSection* sect);
	DWORD alignDisk(DWORD rva);
	PeSection* getSection(int rva, int len);

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

	// Inline helpers
	inline
	char* pestrdup(int rva)
		{	return xstrdup((char*)imageData+rva); }
	inline
	Void rvaToPtr(int rva)
		{	return imageData+rva; }
	inline
	DWORD ptrToRva(void* ptr)
		{	return Void(ptr)-imageData; }
	inline
	bool checkRva(int rva, int length)
		{	return rva+length <= imageSize;	}
	inline 
	bool checkPtr(Void ptr, int length)
		{	return ptr+length <= imageData+imageSize; }
	inline
	WORD& nSections(void)
		{	return fileHeadr->NumberOfSections; }
	inline
	DWORD& imageBase(void)
		{	return optHeadr->ImageBase; }
	inline
	const char* PeSection::name(void)
		{	return (const char*)Name; }
	inline 
	DWORD& PeSection::vSize(void)
		{	return Misc.VirtualSize; }
	inline
	DWORD& PeSection::baseRva(void)
		{	return VirtualAddress; }
	inline
	DWORD PeSection::endRva(void)
		{	return baseRva() + vSize(); }
	inline
	DWORD PeSection::endPage(void)
		{	return ALIGN_PAGE(endRva()); }
	inline
	Void& PeSection::basePtr(void)
		{	return *(Void*)&PointerToRelocations; }
	inline
	Void PeSection::endPtr(void)
		{	return basePtr() + vSize(); }
	inline
	int PeSection::extent(void)
		{	return calcExtent(basePtr(), vSize()); }
	inline
	int sectIdx(PeSection* sect)
		{	return sect-peSects; }
