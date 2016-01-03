#ifndef _PEFILE_H_
#define _PEFILE_H_

namespace PeFile {

	extern Void imageData;
	extern IMAGE_OPTIONAL_HEADER* optHeadr;
	static DWORD baseAddr(void) {
		return optHeadr->ImageBase; }
	static DWORD rvaToAddr(int rva) 	{
		return rva+baseAddr(); }
	static DWORD addrToRva(int addr)	{
		return addr-baseAddr(); }
	static DWORD& entryPoint() {
		return optHeadr->AddressOfEntryPoint; }
	static void subsysGUI(void) {optHeadr->Subsystem
		= IMAGE_SUBSYSTEM_WINDOWS_GUI; }
	Void patchChk(int addr, int len);

	// Load/Save interface
	const char* load(const char* fileName);
	const char* save(const char* fileName);
	void close();

	// Imports interface
	int Import_Find(char* dllName, char* importName);
	int Import_Add(char* dllName, char* importName);

	// Relocations interface
	void Relocs_Add(int rva);
	bool Reloc_Find(int rva);
	int Relocs_Remove(int rva);
	int Relocs_Remove(int rva, int length);
	void Relocs_Report(DWORD* oldRelocs, int nOldRelocs);
	void clearSpace(int rva, int length, int offset);
	extern DWORD* relocs; extern DWORD nRelocs;
	template <class F> Relocs_Find(int rva, int len, F func) { 
		for(int i = 0; i < nRelocs; i++) if((relocs[i] >= rva)
		  &&(relocs[i] < (rva+len))) func(relocs[i]); }
	
	// space allocation inreface
	enum { Type_Data, Type_Bss,	Type_RData,
		Type_Text, Type_ImpDir };
	struct PeBlock {
		WORD type; WORD align;
		DWORD baseRva, length;
		size_t userData; };
	void allocBlocks(PeBlock* blocks, int nBlocks);
}
#endif
