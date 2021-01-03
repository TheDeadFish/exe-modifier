#pragma once

struct PeSymTab
{
	struct ObjSymbol {
		union { char* name; char Name[8];
		struct { DWORD Name1, Name2; }; };
		DWORD Value; WORD Section;WORD Type;
		BYTE StorageClass; BYTE NumberOfAuxSymbols;
	} __attribute__((packed));
	
	

	struct Symbol {
		char* name; u32 rva;
	};
	
	xArray<Symbol> symbol;
	
	
	void add(char* name, u32 rva);
	
	
	struct StrTable {
		xVector<byte> data;
		u32 add(char* str);
	};

	struct Build_t {
		xArray<ObjSymbol> symData;
		StrTable strTab;
		void xwrite(FILE* fp);
		
		
		
		
		bool hasData() { return symData.len
			|| strTab.data.dataSize; }
	};
};
