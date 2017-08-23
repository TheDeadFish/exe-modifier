// relocation fixups
// included by linker.cc
// DeadFish Shitware, 2014

char* getSectionName(WORD section, int offset)
{
	return (section == 0xFFFF) ? xstrfmt("@image:%X", offset) : xstrfmt(
		"%s:%s:%X", sections[section].fileName, sections[section].name, offset);
}

bool undef_symbol_flag;
void undef_symbol(Symbol* symb, WORD section, int offset)
{
	undef_symbol_flag = true;
	char* sectName = getSectionName(section, offset);
	error_msg("undefined symbol: %s, referenced by: %s\n",
		symb->getName(), sectName); free(sectName);
}

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
		DWORD& addr = PeFILE::rvaToPtr(relocRva).dword();
		auto& symb = symbols[reloc.symbol];
		
		const char* symbName = symb.Name ? 
			symb.Name : "##NO NAME##";
		if(symb.section == Type_Undefined)
			undef_symbol(&symb, reloc.section, reloc.offset);

		DWORD symbAddr = symb.getAddr();
		if(reloc.type == Type_DIR32) {
			addr += symbAddr;
			if(symb.section != Type_Absolute)
				PeFILE::Relocs_Add( relocRva );
		} else {
			addr += symbAddr;
			addr -= PeFILE::rvaToAddr(relocRva) + 4;
		}
	}
	
	if(undef_symbol_flag) exit(1);
}
