
void symTab_build(void)
{
	LINKER_ENUM_SYMBOLS(symb,
		if((symb->section != Linker::Type_Relocate)
		&&((!symb->section->isReal() 
		|| !symb->section->baseRva)))
			continue;
		
		u32 rva = symb->getRva(); 
		if(!rva || !symb->Name) continue;
		PeFILE::peFile.symtab.add(symb->Name, rva);
	)
}
