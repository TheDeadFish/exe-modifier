// relocation fixups
// included by linker.cc
// DeadFish Shitware, 2014

void addReloc(WORD type, WORD section, DWORD offset, DWORD symbol)
{
	auto& reloc = xNextAlloc(relocs, nRelocs);
	reloc.type = type;
	reloc.section = section;
	reloc.offset = offset;
	reloc.symbol = symbol;
}

void relocs_fixup(void)
{
	for(auto& reloc : Range(relocs, nRelocs))
	{
		if(reloc.type == (WORD)-1)
			continue;
		DWORD baseRva = (reloc.section == 0xFFFF) ?
			0 : sections[reloc.section].baseRva;
		DWORD relocRva = baseRva + reloc.offset;
		DWORD& addr = (PeFile::imageData + relocRva).dword();
		auto& symb = symbols[reloc.symbol];
		
		const char* symbName = symb.Name ? 
			symb.Name : "##NO NAME##";
		if(symb.section == Type_Undefined)
			fatal_error("undefined symbol: %s\n", symbName);

		DWORD symbAddr = symb.getAddr();
		if(reloc.type == Type_DIR32) {
			addr += symbAddr;
			if(symb.section != Type_Absolute)
				PeFile::Relocs_Add( relocRva );
		} else {
			addr += symbAddr;
			addr -= PeFile::rvaToAddr(relocRva) + 4;
		}
	}
}
