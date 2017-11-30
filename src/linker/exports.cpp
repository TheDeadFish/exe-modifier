
struct ExportSlot
{
	cch* name;
	DWORD symbol;
	DWORD offset;
	DWORD oldRva;
};

xarray<ExportSlot> exports;

struct ExportSymb
{
	cch* name;
	DWORD symbol;
};

xarray<ExportSymb> expSymb;

void addExport(char* name, uint ord, 
	DWORD symbol, DWORD offset, DWORD oldRva)
{
	if(ord) { name = xstrfmt("#%d", ord); }
	else { name = xstrdup(name); }
	exports.push_back(name, symbol, offset, oldRva);
}

int findSymbolExp(const char* Name)
{
	if(Name != NULL)
	  for(int i = 0; i < nSymbols; i++) {
		char* nm = symbols[i].Name;
		if(nm && *nm == '_') { 
		  nm = strScmp(nm+1, Name);
		  if(nm && is_one_of(*nm, 0, '@'))
			  return i;
		}
	}
	return -1;
}

void exports_symbfix()
{
	for(auto& slot : exports)
	{
		// match symbol
		auto& symb = symbols[slot.symbol];
		if(symb.section == Type_Undefined) {
			int iSymb = findSymbolExp(symb.Name);
			if(iSymb < 0) continue; slot.symbol = iSymb; }
		
		// create symbol to original export
		if(slot.oldRva) { xstr name(symbcat
		(symbols[slot.symbol].Name, "_org"));
		addSymbol(name, Type_Relocate, -1,
			PeFILE::rvaToAddr(slot.oldRva)); }
	}
}

void exports_resolve()
{
	// resolve new exports
	for(auto& slot : exports) {
		auto& symb = symbols[slot.symbol];
		if(symb.section == Type_Undefined)
			undef_symbol(&symb, 0, 0);
		PeFILE::peExp.setRva(slot.name, 
			symb.getRva() + slot.offset);
	}
	
	// update export symbols
	for(auto& slot : expSymb) {
		symbols[slot.symbol].value = 
			PeFILE::peExp.find(slot.name)->rva;
	}
}

void exports_addSymb(int symb, char* importName)
{
	expSymb.push_back(xstrdup(importName), symb);
}

int exports_getExpSym(
	char*& dllName, char*& importName)
{
	// lookup export name
	if((!PeFILE::peExp.dllName)
	||(stricmp(PeFILE::peExp.dllName, dllName))) return -1;
	auto* slot = PeFILE::peExp.find(importName);
	if(!slot) { fatal_error("exported function"
		" does not exist: %s", importName); }
	
	// handle export forwarding
	if(slot->frwd) { char* dotPos = strchr(slot->frwd, '.');
	if(!dotPos) fatal_error("bad export forwarder: %s", importName);
	dotPos = importName; dllName = xstrdup(slot->frwd, dotPos-slot->frwd);
	return -2; }
	
	// create export symbol
	int symb = addSymbol(NULL, Type_Relocate, 0, 0);
	exports_addSymb(symb, importName);
	return symb;
}
