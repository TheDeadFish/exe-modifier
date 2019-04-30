	
void mark_section(Section* sect);
void mark_reloc(xarray<Reloc> relocs)
{
	for(auto& reloc : relocs) {
		auto* symb = reloc.getSymb();
		mark_section(symb->getSect()); }
}

void mark_section(Section* sect)
{
	if(!sect || sect->keep) return;
	sect->keep = 1;
	mark_reloc(sect->rlcs());
}

void gc_sections(void)
{
	// mark keep sections
	for(char* keep : keep_list) {
		auto* symb = findSymbol(keep);
		mark_section(symb->getSect());
		
		// mark matching sections
		LINKER_ENUM_SECTIONS(sect, 
			if(strScmp(sect->name, keep))
				mark_section(sect));
	}
	
	// mark -1 refernced sections
	mark_reloc({relocs, nRelocs});
	for(auto& slot : exports) {
		auto* symb = slot.symbol;
		mark_section(symb->getSect()); }
		
	// kill unreferenced sections
	LINKER_ENUM_SECTIONS(sect, 
		if(!sect->keep) destroy_section(*sect))
}
