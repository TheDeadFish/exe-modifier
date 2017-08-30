
struct ExportSlot
{
	cch* name;
	DWORD symbol;
	DWORD offset;
};

xarray<ExportSlot> exports;

void addExport(char* name, uint ord, 
	DWORD symbol, DWORD offset)
{
	auto& slot = exports.xnxalloc();
	slot.symbol = symbol; slot.offset = offset;
	if(ord) { slot.name = xstrfmt("#%d", ord); }
	else { slot.name = xstrdup(name); }
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
