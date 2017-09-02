
struct ExportSlot
{
	cch* name;
	DWORD symbol;
	DWORD offset;
	DWORD oldRva;
};

xarray<ExportSlot> exports;

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
	for(auto& slot : exports) {
		auto& symb = symbols[slot.symbol];
		if(symb.section == Type_Undefined)
			undef_symbol(&symb, 0, 0);
		PeFILE::peExp.setRva(slot.name, PeFILE::
			addrToRva(symb.getAddr()) + slot.offset);
	}
}
