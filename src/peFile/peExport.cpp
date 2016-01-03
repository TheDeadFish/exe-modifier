
struct ExportFunc
{
	char* frwdName;
	DWORD slotRva;
	DWORD exportRva;
};

struct ExportName
{
	char* name;
	int ordinal;
};

struct ExportDir
{
	char* DllName;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	WORD OrdinalBase;
	ExportFunc* expFuncs;
	int nExpFuncs;
	ExportName* exports;
	int nExports;
} exportDir;

const char* Exports_Load(void)
{
	if(dataDir[IDE_EXPORT].Size == 0)
		return NULL;
		
	// check export directory
	int expRva = dataDir[IDE_EXPORT].VirtualAddress;
	int expSize = dataDir[IDE_EXPORT].Size;
	PIMAGE_EXPORT_DIRECTORY expBase = rvaToPtr(expRva);
	RangeChk expChk(expBase, Void(expBase)+expSize);
	if((!sectChk.check(expBase, expSize))
	||(!expChk.check(expBase, sizeof(IMAGE_EXPORT_DIRECTORY)))
	||(expChk.checkStr(rvaToPtr(expBase->Name)) < 0))
		return "Exports_Load: bad directory";
		
	// check export tables
	int nExpFuncs = expBase->NumberOfFunctions;
	int nExports = expBase->NumberOfNames;
	DWORD* expFuncs = rvaToPtr(expBase->AddressOfFunctions);
	DWORD* expNames = rvaToPtr(expBase->AddressOfNames);
	WORD* expOrds = rvaToPtr(expBase->AddressOfNameOrdinals);
	if((!expChk.check(expFuncs, nExpFuncs*4))
	||(!expChk.check(expNames, nExports*4))
	||(!expChk.check(expOrds, nExports*2)))
		return "Exports_Load: bad tables";
	
	// check table contents
	for(DWORD func : Range(expFuncs, nExpFuncs)) {
		Void pfunc = rvaToPtr(func);
		if((expChk.check(pfunc))
		&&(!expChk.checkStr(pfunc) < 0))
			return "Exports_Load: bad AddressOfFunctions"; 
	}
	for(int i : Range(0, nExports))	{
		if((expChk.checkStr(rvaToPtr(expNames[i])) < 0)
		||(expOrds[i] >= nExpFuncs))
			return "Exports_Load: bad AddressOfNames/AddressOfNameOrdinals"; 
	}
	
	// initialize ExportDir
	exportDir.DllName = pestrdup(expBase->Name);
	exportDir.TimeDateStamp = expBase->TimeDateStamp;
	exportDir.MajorVersion = expBase->MajorVersion;
	exportDir.MinorVersion = expBase->MinorVersion;
	exportDir.OrdinalBase = expBase->Base;
	exportDir.expFuncs = xMalloc(nExpFuncs);
	exportDir.nExpFuncs = nExpFuncs;
	exportDir.exports = xMalloc(nExports);
	exportDir.nExports = nExports;
	for(int i : Range(0, nExpFuncs)) {
		char* pfunc = rvaToPtr(expFuncs[i]);
		exportDir.expFuncs[i].slotRva = ptrToRva(&expFuncs[i]);
		bool forwarder = expChk.checkStr(pfunc) >= 0;
		exportDir.expFuncs[i].exportRva = !forwarder ? expFuncs[i] : 0;
		exportDir.expFuncs[i].frwdName = forwarder ? xstrdup(pfunc) : NULL; }
	for(int i : Range(0, nExports)) {
		exportDir.exports[i].name = pestrdup(expNames[i]);
		exportDir.exports[i].ordinal = expOrds[i]; }
	return 0;
}

void Exports_Free()
{
	for(int i = 0; i < exportDir.nExpFuncs; i++)
	  free(exportDir.expFuncs[i].frwdName);
	for(int i = 0; i < exportDir.nExports; i++)
	  free(exportDir.exports[i].name);
	free(exportDir.DllName);
	free(exportDir.expFuncs); 
	free(exportDir.exports);
	memset(&exportDir, 0, sizeof(exportDir));
}

int Exports_Find(char* exportName) {
	for(int i = 0; i < exportDir.nExports; i++) {
	  if(!strcmp(exportName, exportDir.exports[i].name))
		return i; }
	return -1; }
ExportFunc* Exports_FindFunc(char* exportName) {
	int index = Exports_Find(exportName);
	if(index < 0) return NULL;
	return &exportDir.expFuncs[
		exportDir.exports[index].ordinal]; }
int	Exports_HasFunc(char* dllName, char* exportName) 
{
	if((exportDir.DllName == NULL)
	||((dllName && stricmp(dllName, exportDir.DllName))))
		return 0;
	ExportFunc* pFunc = Exports_FindFunc(exportName);
	return pFunc ? pFunc->exportRva : 0;
}
