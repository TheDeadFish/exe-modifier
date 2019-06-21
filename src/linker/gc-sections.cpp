	
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

void rref_symb(Section* sect, Symbol* symb)
{
	FOR_REV(auto& reloc, sect->rlcs(), 
		if(reloc.type != WORD(-1)) break;
		if(reloc.symbol == symb) return; );
	sect->addReloc(-1, 0, symb);
}

void rref_sect(Section* sect)
{
	for(auto& reloc : sect->rlcs()) {
		Symbol* symb = reloc.symbol;
		if(symb->section->isReal()) { rref_symb(
			symb->section, sect->symbol); }
	}
}

void gc_sections(void)
{
	// create reverse refs
	LINKER_ENUM_SECTIONS(sect, 
		if(sect->rref) rref_sect(sect));

	// mark keep sections
	for(char* keep : keep_list) {
		auto* symb = findSymbol(keep);
		if(symb) mark_section(symb->getSect());
		
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

void gc_sections2(void)
{
	// remark sections
	mark_reloc({relocs, nRelocs});
	LINKER_ENUM_SECTIONS(sect, 
		mark_reloc(sect->rlcs()));
	
	// kill unreferenced sections
	LINKER_ENUM_SECTIONS(sect, 
		if(!sect->keep) destroy_section(*sect))	
}
