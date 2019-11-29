
void symTab_build(void)
{
#if 0
	LINKER_ENUM_SYMBOLS(symb,
		Section* sect = symb->section;
		if((sect != Linker::Type_Relocate)
		&&(!sect->isReal() || !sect->baseRva
		|| sect->symbol == symb)) continue;
		
		u32 rva = symb->getRva(); 
		if(!rva || !symb->Name) continue;
		PeFILE::peFile.symtab.add(symb->Name, rva);
	)
#endif
}
