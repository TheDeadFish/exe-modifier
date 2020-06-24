
struct ExportSlot
{
	cch* name;
	union { cch* frwd;
		Symbol* symbol; };
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
	cch* frwd, DWORD offset, DWORD oldRva)
{
	if(ord) { name = xstrfmt("#%d", ord); }
	else { name = xstrdup(name); }
	frwd = xstrdup(frwd);
	exports.push_back(name, frwd, offset, oldRva);
}

int expSymCmp(cch* frwd, cch* symb)
{
	for(int i = 0;; i++) { AGAIN:
		if(!symb[i]) return !frwd[i];
		if(symb[i] != frwd[i]) {
			if(symb[i] == '_'){ symb++; goto AGAIN; }
			if(symb[i] == '@') return frwd[i];
			return 1;
		}
	}
}

Symbol* findSymbolExp(const char* Name)
{
	return symb_find(Name, expSymCmp);
}

void exports_symbfix()
{
	for(auto& slot : exports)
	{
		// match symbol
		slot.symbol = findSymbolExp(slot.frwd);
		if(!slot.symbol) continue;
		
		// create symbol to original export
		if(slot.oldRva) { xstr name(symbcat
		(slot.symbol->Name, "_org"));
		addSymbol(name, Type_Relocate, 0,
			PeFILE::rvaToAddr(slot.oldRva)); }
	}
}

void exports_resolve()
{
	// update export symbols
	for(auto& slot : expSymb) {
		slot.symbol->value = 
			PeFILE::peExp.find(slot.name)->rva;
	}

	// resolve new exports
	for(auto& slot : exports) {
		if(!slot.symbol) { undef_symbol_flag = true;
			error_msg("undefined export: %s\n", slot.name);
			continue; }
	
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

bool needSymbol(const char* name)
{
	// check existing symbol
	auto* symb = findSymbol(name);
	if(symb) return symb->section == Type_Undefined;
	
	// check matching export
	for(auto& slot : exports) {
		if(!expSymCmp(slot.frwd, name))
			return true; 
	} return false;
}
