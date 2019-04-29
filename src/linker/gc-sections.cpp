
void gc_sections(void)
{
	// mark keep sections
	char* sectMask = xMalloc(nSections);
	memset(sectMask, 0, nSections);
	for(char* keep : keep_list) {
		auto* symb = findSymbol2(keep);
		if(symb && !isNeg(symb->section)) {
			sectMask[symb->section] = 1; }
		for(int j = 0; j < nSections; j++) {
		  if(strScmp(sections[j]->name, keep))
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
	  &&(sections[i]->isAlive()))
	{
		sectMask[i] = 2;
		for(auto& reloc : Range(sections[i]
		  ->relocs, sections[i]->nReloc)) 
		{
			auto* symbol = reloc.getSymb();
			if(symbol == NULL) continue;
			if((sectMask[symbol->section] == 0)
			&&(sections[i]->isAlive())) {
				sectMask[symbol->section] = 1;
					maskChange = true; }
		}
	}
	if(maskChange == true)
		goto RECHECK_MASK;
		
	// kill unreferenced sections
	for(int i = 0; i < nSections; i++)
	  if((sectMask[i] != 2)
	  &&(sections[i]->isAlive())) {
		assert(sectMask[i] == 0);
		destroy_section(*sections[i]); 
	}
}
