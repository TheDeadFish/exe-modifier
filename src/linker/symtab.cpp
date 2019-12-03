

void symTab_symb(Symbol* symb)
{
	Section* sect = symb->section;
	if((sect != Linker::Type_Relocate)
	&&(!sect->isReal() || !sect->baseRva
	|| sect->symbol == symb)) return;
	
	u32 rva = symb->getRva(); 
	if(!rva || !symb->Name) return;
	PeFILE::peFile.symtab.add(symb->Name, rva);
}

void symTab_build(void)
{
	LINKER_ENUM_SYMBOLS(symb,
		if(!symb->section->isReal())
			symTab_symb(symb);
	)

	LINKER_ENUM_SECTIONS(sect, 
		Symbol* symb = sect->symbol;
		for(;symb;symb=symb->sectLst) {
			symTab_symb(symb);
		}
	)
}
