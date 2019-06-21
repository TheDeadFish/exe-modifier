
Symbol* addImport(const char* Name,
	const char* dllName, const char* importName)
{
	assert(0); return 0;

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

void import_bad(Section* idata5, cch* name)
{
	fatal_error("import_bad, %s\n", name);
}


void imports_parse(void)
{
	LINKER_ENUM_SECTIONS(sect, 
	
		// check for import
		if(!sect->nameIs(".idata$7")) continue;
		auto idata5 = sect->next;
		if(!idata5->nameIs(".idata$5")) continue;
		auto idata6 = idata5->next->next;
		if(idata5->nReloc != 1) import_bad(NULL, ".idata$5");
		if(sect->nReloc != 1) import_bad(idata5, ".idata$7");
		
		// get import name (.idata$6)
		if((!idata6->nameIs(".idata$6"))
		||( !nullchk(idata6->rawData+2, idata6->endPtr())))
			import_bad(idata5, ".idata$6");
		char* importName = idata6->rawData+2;
		
		// get import directory (.idata$2)
		auto idata2 = sect->relocs->getSect();
		if(!idata2 || !idata2->nameIs(".idata$2") ||
		(idata2->length != 20)||(idata2->nReloc != 3))
			import_bad(idata5, ".idata$2");
			
		// get module name (.idata7)
		auto idata7b = idata2->relocs[1].getSect();
		if(!idata7b->nameIs(".idata$7")
		||( !nullchk(idata7b->rawData, idata7b->endPtr())))
			import_bad(idata5, ".idata$7b");
		char* dllName = idata7b->rawData;
		
		// check for import from self
		auto expsymb = exports_getExpSym(dllName, importName);
		if(!expsymb) { PeFILE::Import_Add(dllName, importName);
			impSects.push_back(dllName, importName, idata5); continue; }
			
		// resolve import from self
		idata5->relocs->symbol = expsymb;
		idata5->relocs->type = x64Mode() ? Type_DIR64 : Type_DIR32;
		idata5->type = PeSecTyp::RData|Type_Keep;
	)
		
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
}
