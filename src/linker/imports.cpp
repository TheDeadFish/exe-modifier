
Symbol* addImport(const char* Name,
	const char* dllName, const char* importName)
{
#if 0
	// todo - generate import symbol
	assert(Name != NULL);

	auto* symbol = addSymbol(Name, Type_Import, 0, 0);
	if(symbol)
		makeImportSymbol(*symbol, dllName, importName);
	return symbol;
#endif
}

struct ImportSects {
	char *dllName, *importName; 
	Section* section; };
xarray<ImportSects> impSects;

void imports_resolve(void)
{
	for(auto& is : impSects) {
		is.section->baseRva = PeFILE::
			Import_Find(is.dllName, is.importName); }
}

void import_fixReloc(byte* base, 
	Symbol* iatSymb, Symbol* expsymb,
	Reloc* relocs, int nRelocs)
{
	for(auto& reloc : Range(relocs, nRelocs)) 
	if(reloc.getSymb() == iatSymb) 
	{
		byte* base2 = base ? base :
		PeFILE::peFile.rvaToSect(reloc.offset, 0)->data;
		byte* rpos = base2+reloc.offset;
		reloc.symbol = expsymb;

		// 0x25FF: jmp indirect
		if(RW(rpos,-2) == 0x25FF) { RW(rpos,-2)
			= 0xE990; reloc.type = Type_REL32; }
		
		// 0x15FF: call indirect
		ei(RW(rpos,-2) == 0x15FF) { RW(rpos,-2)
			= 0xE890; reloc.type = Type_REL32; }
		
		// 0xA1: move eax, [address]
		ei(rpos[-1] == 0xA1) { rpos[-1] = 0xB8; }
			
		// 0x??8B: move ea?, [address]
		ei((RW(rpos,-2) & 0xC7FF) == 0x58B)
		{
			int r = (RW(rpos,-2) >> 11);
			RW(rpos,-2) = 0xB890 + (r << 8);
		}
		
		else {
			fatal_error("import_fixReloc: failed");
		}
		
	}
};


void imports_parse(void)
{
	LINKER_ENUM_SYMBOLS(iatsym,
	  if((iatsym->Name && strScmp(iatsym->Name, "__imp_"))
	  &&(int(iatsym->section) >= 0)
	  &&(strScmp(sections[iatsym->section]->name, ".idata$")))
	{
		// check .idata$5
		auto idata5 = sections[iatsym->section];
		if((iatsym->section == 0)
		||(strcmp(idata5->name, ".idata$5"))
		||(idata5->nReloc != 1)) {
	ERROR_IDATA5: fatal_error(
			"imports_parse: .idata$5 bad, %s\n", iatsym->Name); }
		auto idata5_symbol = idata5->relocs->getSymb();
		int idata6_section = idata5_symbol->section;
		if(idata6_section == (WORD)-1)
			goto ERROR_IDATA5;
		
		// get/check .idata6 (import name)
		auto idata6 = sections[idata6_section];
		if((strcmp(idata6->name, ".idata$6"))
		||( !nullchk(idata6->rawData+2, idata6->endPtr()))) {
			fatal_error("imports_parse: .idata$6 bad, %s\n", 
				iatsym->Name); }
		char* importName = idata6->rawData+2;
		
		// get/check .idata7
		auto idata7 = sections[iatsym->section-1];
		if((strcmp(idata7->name, ".idata$7"))
		||(idata7->nReloc != 1)) {
	ERROR_IDATA7: fatal_error(
			"imports_parse: .idata$7 bad, %s\n", iatsym->Name); }
		auto idata7_symbol = idata7->relocs->symbol;
		int idata2_section = idata7_symbol->section;
		if(idata2_section == (WORD)-1)
			goto ERROR_IDATA7;

		// get/check .idata2 (import directory)
		auto idata2 = sections[idata2_section];
		if((strcmp(idata2->name, ".idata$2"))
		||(idata2->length != 20)
		||(idata2->nReloc != 3)) {
	ERROR_IDATA2: fatal_error(
			"imports_parse: .idata$2 bad, %s\n", iatsym->Name); }
		auto idata2_symbol = idata2->relocs[1].symbol;
		int idata7b_section = idata2_symbol->section;
		if(idata7b_section == (WORD)-1)
			goto ERROR_IDATA2;
			
		// get/check .data7(b) (dll name)
		auto idata7b = sections[idata7b_section];
		if((strcmp(idata7b->name, ".idata$7"))
		||( !nullchk(idata7b->rawData, idata7b->endPtr()))) {
			fatal_error("imports_parse: .idata$7b bad, %s\n", 
				iatsym->Name); }
		char* dllName = idata7b->rawData;
		
		// check for import from self
		auto expsymb = exports_getExpSym(dllName, importName);
		if(!expsymb) { PeFILE::Import_Add(dllName, importName);
			impSects.push_back(dllName, importName, idata5); continue; }
		
#if 0
		// redirect relocations
		import_fixReloc(0, iatsym, expsymb, relocs, nRelocs);
		LINKER_ENUM_SECTIONS(sect,
			if(sect->isExec()) { import_fixReloc(sect->rawData,
				iatsym, expsymb, sect->relocs, sect->nReloc); })
			
		// redirect thunk symbols
		if(thunkSect >= 0) {
			LINKER_ENUM_SYMBOLS(symb,
			if(symb->section == thunkSect) {
				symb->section = Type_Relocate;
				exports_addSymb(symb, importName); }
			destroy_section(*sections[thunkSect]); )
		}
#endif
	})
}
