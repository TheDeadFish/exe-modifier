
struct ExportSlot
{
	cch* name;
	Symbol* symbol;
	DWORD offset;
	DWORD oldRva;
};

xarray<ExportSlot> exports;

struct ExportSymb
{
	cch* name;
	Symbol* symbol;
};

xarray<ExportSymb> expSymb;

void addExport(char* name, uint ord, 
	Symbol* symbol, DWORD offset, DWORD oldRva)
{
	if(ord) { name = xstrfmt("#%d", ord); }
	else { name = xstrdup(name); }
	exports.push_back(name, symbol, offset, oldRva);
}

Symbol* findSymbolExp(const char* Name)
{
	if(Name != NULL)
		LINKER_ENUM_SYMBOLS(symb,
		char* nm = symb->Name;
		if(nm && *nm == '_') { 
		  nm = strScmp(nm+1, Name);
		  if(nm && is_one_of(*nm, 0, '@'))
			  return symb;
		}
	);
	return NULL;
}

void exports_symbfix()
{
	for(auto& slot : exports)
	{
		// match symbol
		auto& symb = *slot.symbol;
		if(symb.section == Type_Undefined) {
			auto* iSymb = findSymbolExp(symb.Name);
			if(!iSymb) continue; slot.symbol = iSymb; }
		
		// create symbol to original export
		if(slot.oldRva) { xstr name(symbcat
		(slot.symbol->Name, "_org"));
		addSymbol(name, Type_Relocate, 0,
			PeFILE::rvaToAddr(slot.oldRva)); }
	}
}

void exports_resolve()
{
	// resolve new exports
	for(auto& slot : exports) {
		auto& symb = *slot.symbol;
		if(symb.section == Type_Undefined)
			undef_symbol(&symb, 0, 0);
		PeFILE::peExp.setRva(slot.name, 
			symb.getRva() + slot.offset);
	}
	
	// update export symbols
	for(auto& slot : expSymb) {
		slot.symbol->value = 
			PeFILE::peExp.find(slot.name)->rva;
	}
}

void exports_addSymb(Symbol* symb, char* importName)
{
	expSymb.push_back(xstrdup(importName), symb);
}

Symbol* exports_getExpSym(
	char*& dllName, char*& importName)
{
	// lookup export name
	if((!PeFILE::peExp.dllName)
	||(stricmp(PeFILE::peExp.dllName, dllName))) return 0;
	auto* slot = PeFILE::peExp.find(importName);
	if(!slot) { fatal_error("exported function"
		" does not exist: %s", importName); }
	
	// handle export forwarding
	if(slot->frwd) { char* dotPos = strchr(slot->frwd, '.');
	if(!dotPos) fatal_error("bad export forwarder: %s", importName);
	importName = dotPos+1; dllName = xstrfmt("%v.dll", 
		slot->frwd.data, dotPos-slot->frwd); return 0; }
	
	// create export symbol
	auto symb = addSymbol(NULL, Type_Relocate, 0, 0);
	exports_addSymb(symb, importName);
	return symb;
}
