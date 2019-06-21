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
