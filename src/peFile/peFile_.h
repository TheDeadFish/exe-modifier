#ifndef _PEFILE__H_
#define _PEFILE__H_
#include "..\exe_mod.h"
#include "peFile.h"
#include "peBlock.h"
#include "peImport.h"

namespace PeFILE {

	extern PeFile peFile;
	extern PeImport peImp;
	
	
	
	static DWORD baseAddr(void) { return peFile.ImageBase; }
	static DWORD rvaToAddr(int rva) { return rva+baseAddr(); }
	static DWORD addrToRva(int addr) { return addr-baseAddr(); }
	static DWORD& entryPoint() { return peFile.AddressOfEntryPoint; }
	
	
	
	static void subsysGUI(void) {peFile.Subsystem
		= IMAGE_SUBSYSTEM_WINDOWS_GUI; }
		
	static Void patchChk(int addr, int len) { return peFile.patchChk(addr, len); }
	static DWORD ptrToRva(void* p) { return peFile.ptrToRva(p); }
	static Void rvaToPtr(DWORD rva) { return peFile.rvaToPtr(rva); }

	// Load/Save interface
	const char* load(const char* fileName);
	const char* save(const char* fileName);
	void close();

	// Imports interface
	int Import_Find(char* dllName, char* importName);
	int Import_Add(char* dllName, char* importName);

	// Relocations interface
	static void Relocs_Add(int rva) { peFile.relocs.Add(rva); }
	static bool Reloc_Find(int rva) { return peFile.relocs.Find(rva); }
	static void Relocs_Remove(int rva) { peFile.relocs.Remove(rva); }
	static void Relocs_Remove(int rva, int length) {
		peFile.relocs.Remove(rva, length); }
	static void Relocs_Move(u32 rva, u32 length, int delta) {
		peFile.relocs.Move(rva, length, delta); }
	
	void clearSpace(int rva, int length, int offset);
	extern DWORD* relocs; extern DWORD nRelocs;
	template <class F> Relocs_Find(int rva, int len, F func) { 
		for(int i = 0; i < nRelocs; i++) if((relocs[i] >= rva)
		  &&(relocs[i] < (rva+len))) func(relocs[i]); }
	
	// space allocation inreface
	void allocBlocks(PeBlock* blocks, int nBlocks);
}
#endif
