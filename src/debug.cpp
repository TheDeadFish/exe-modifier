#include "stdafx.h"
#include "exe_mod.h"

void dump_sections(FILE* fp)
{
	LINKER_ENUM_SECTIONS(sect,
		if(!sect->baseRva) continue;
		fprintf(fp, "%04X: %04X, %02X, %s\n", 
			sect->baseRva, sect->length, 
			sect->type, sect->name);
	)
}

void dump_symbols(FILE* fp)
{
	LINKER_ENUM_SYMBOLS(symb,
		if(!symb->section->isReal()
		&&(symb->section != Linker::Type_Relocate))
			continue;
		u32 rva = symb->getRva();
		if(rva == 0) continue;
		cch* name = symb->getName();
		printf("%X: %s\n", rva, name);
	)
}
