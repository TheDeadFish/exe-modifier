
void gc_sections(void)
{
	// mark keep sections
	char* sectMask = xMalloc(nSections);
	memset(sectMask, 0, nSections);
	for(int i = 0; keep_list[i]; i++) {
		int symbol = findSymbol(keep_list[i]);
		if((symbol >= 0)
		&&(int(symbols[symbol].section) >= 0))
			sectMask[symbols[symbol].section] = 1;
		for(int j = 0; j < nSections; j++) {
		  if(strScmp(sections[j].name, keep_list[i]))
			sectMask[j] = 1; }
	}
	
	// mark -1 refernced sections
	for(auto& reloc : Range(relocs, nRelocs))
	  if(int(symbols[reloc.symbol].section) >= 0)
		 sectMask[symbols[reloc.symbol].section] = 1;
	for(auto& slot : exports)
	  if(int(symbols[slot.symbol].section) >= 0)
		sectMask[symbols[slot.symbol].section] = 1;
	
	// mark referenced sections
RECHECK_MASK:
	bool maskChange = false;
	for(int i = 0; i < nSections; i++)
	  if((sectMask[i] == 1)
	  &&(int(sections[i].type) >= 0))
	{
		sectMask[i] = 2;
		for(auto& reloc : Range(sections[i]
		  .relocs, sections[i].nReloc)) {
			auto& symbol = symbols[reloc.symbol];
			if(int(symbol.section) < 0)
				continue;
			if((sectMask[symbol.section] == 0)
			&&(int(sections[symbol.section].type) >= 0)) {
				sectMask[symbol.section] = 1;
					maskChange = true; }
		}
	}
	if(maskChange == true)
		goto RECHECK_MASK;
		
	// kill unreferenced sections
	for(int i = 0; i < nSections; i++)
	  if((sectMask[i] != 2)
	  &&(int(sections[i].type) >= 0)) {
		assert(sectMask[i] == 0);
		destroy_section(sections[i]); 
	}
}
