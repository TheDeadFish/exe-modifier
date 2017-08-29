
void makeImportSymbol(Symbol& iatsym,
	const char* dllName, const char* importName)
{
	int totalSize = strlen(iatsym.Name) + 
		strlen(dllName) + strlen(importName);
	char* symName = xmalloc(totalSize+30);
	sprintf(symName, "%s%c%s%c%s", iatsym.Name, 
		0, dllName, 0, importName);
	::free(iatsym.Name);
	iatsym.Name = symName;
	iatsym.section = Type_Import;
}

int addImport(const char* Name,
	const char* dllName, const char* importName)
{
	int symbol = addSymbol(Name, Type_Import, 0, 0);
	if(symbol >= 0)
		makeImportSymbol(symbols[symbol], dllName, importName);
	return symbol;
}

void imports_resolve(void)
{
	for(auto& symbol : Range(symbols, nSymbols))
	  if(symbol.section == Type_Import)
	{
		char* dllName = strchr(symbol.Name, '\0')+1;
		char* impName = strchr(dllName, '\0')+1;
		int impRva = PeFILE::Import_Find(dllName, impName);
		assert(impRva > 0);
		symbol.section = Type_Relocate;
		symbol.value = PeFILE::rvaToAddr(impRva);
	}
}

void imports_parse(void)
{
	for(auto& iatsym : Range(symbols, nSymbols))
	  if((iatsym.Name && strScmp(iatsym.Name, "__imp__"))
	  &&(int(iatsym.section) >= 0)
	  &&(strScmp(sections[iatsym.section].name, ".idata$")))
	{
		// check .idata$5
		auto idata5 = &sections[iatsym.section];
		if((iatsym.section == 0)
		||(strcmp(idata5->name, ".idata$5"))
		||(idata5->nReloc != 1)) {
	ERROR_IDATA5: fatal_error(
			"imports_parse: .idata$5 bad, %s\n", iatsym.Name); }
		int idata5_symbol = idata5->relocs->symbol;
		int idata6_section = symbols[idata5_symbol].section;
		if(idata6_section == (WORD)-1)
			goto ERROR_IDATA5;
		
		// get/check .idata6 (import name)
		auto idata6 = &sections[idata6_section];
		if((strcmp(idata6->name, ".idata$6"))
		||( !nullchk(idata6->rawData+2, idata6->endPtr()))) {
			fatal_error("imports_parse: .idata$6 bad, %s\n", 
				iatsym.Name); }
		char* importName = idata6->rawData+2;
		
		// get/check .idata7
		auto idata7 = idata5-1;
		if((strcmp(idata7->name, ".idata$7"))
		||(idata7->nReloc != 1)) {
	ERROR_IDATA7: fatal_error(
			"imports_parse: .idata$7 bad, %s\n", iatsym.Name); }
		int idata7_symbol = idata7->relocs->symbol;
		int idata2_section = symbols[idata7_symbol].section;
		if(idata2_section == (WORD)-1)
			goto ERROR_IDATA7;

		// get/check .idata2 (import directory)
		auto idata2 = &sections[idata2_section];
		if((strcmp(idata2->name, ".idata$2"))
		||(idata2->length != 20)
		||(idata2->nReloc != 3)) {
	ERROR_IDATA2: fatal_error(
			"imports_parse: .idata$2 bad, %s\n", iatsym.Name); }
		int idata2_symbol = idata2->relocs[1].symbol;
		int idata7b_section = symbols[idata2_symbol].section;
		if(idata7b_section == (WORD)-1)
			goto ERROR_IDATA2;
			
		// get/check .data7(b) (dll name)
		auto idata7b = &sections[idata7b_section];
		if((strcmp(idata7b->name, ".idata$7"))
		||( !nullchk(idata7b->rawData, idata7b->endPtr()))) {
			fatal_error("imports_parse: .idata$7b bad, %s\n", 
				iatsym.Name); }
		char* dllName = idata7b->rawData;
		
		// fix thunk reloc
		int thunkSect = -1;
		for(int i = idata6_section; --i >= 0;)
		if(!strcmp(sections[i].name, ".text")) {
			DWORD& relocSymb = sections[i].relocs->symbol;
			if((sections[i].nReloc == 1)
			&&( symbols[relocSymb].section == iatsym.section )) {
				relocSymb = &iatsym-symbols; 
				thunkSect = i; }
			break; }
		
		// register import
		if(int impRva = PeFILE::Import_Add(dllName, importName))
		{
			// redirect thunk symbols
			if(thunkSect >= 0) {
				for(auto& symb : Range(symbols, nSymbols))
				  if(symb.section == thunkSect)	{
					symb.section = Type_Relocate;
					if(symb.value != 0)
						fatalError("imports_parse: bad symbol");
					symb.value = PeFILE::rvaToAddr(impRva); }
				destroy_section(sections[thunkSect]); 
			}
		
		} else {
			makeImportSymbol(iatsym, dllName, importName);
		}
	}
	
	// destroy import data
	for(auto& section : Range(sections, nSections))
	  if(strScmp(section.name, ".idata"))
		destroy_section(section);
}
