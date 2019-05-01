
struct ObjSymbol
{
	DWORD Name1;
	DWORD Name2;
	DWORD Value;
	WORD Section;
	WORD Type;
	BYTE StorageClass;
	BYTE NumberOfAuxSymbols;
} __attribute__((packed));

struct ObjRelocs
{
	DWORD offset;
	DWORD symbol;
	WORD type;
} __attribute__((packed));

static
DWORD sect_align(DWORD flags) {
	DWORD align = (flags>>20)&15;
	if(align != 0) align = 1 << (align-1);
	return align;
}

void object_load(const char* fileName,
	Void objFile, DWORD objSize)
{
	// validate header
	fileName = xstrdup(fileName);
	Void objLimit = objFile + objSize;
	IMAGE_FILE_HEADER* objHeadr = objFile;
	IMAGE_SECTION_HEADER* objSects = (Void)(objHeadr+1);	
	if(objLimit < (objHeadr+1))
		file_corrupt("object", fileName);
	DWORD nSects = objHeadr->NumberOfSections;
	if(objLimit < (objSects+nSects))
		file_corrupt("object", fileName);
	DWORD nSymbols = objHeadr->NumberOfSymbols;
	ObjSymbol* objSym = objFile + objHeadr->PointerToSymbolTable;
	char* strTab = (char*)(objSym+nSymbols);
	if(objLimit < (strTab+4))
		file_corrupt("object", fileName);
		
	// read sections
	Section** sectMapp = xCalloc(nSects);
	for(int i = 0; i < nSects; i++)
	{
		// reference section members
		auto& sect = objSects[i];
		DWORD& size = sect.SizeOfRawData;
		DWORD& flags = sect.Characteristics;
		DWORD& data = sect.PointerToRawData;
		
		// verify section
		if(flags & IMAGE_SCN_MEM_DISCARDABLE) continue;
		if(sect.VirtualAddress) file_bad("object", fileName);
		if(data&&((objSize < data)||(size > (objSize-data))))
			file_bad("object", fileName);
			
		// get section name
		char* Name = (char*)objSects[i].Name;
		if(Name[0] == '/') { Name = strTab + atoi(Name+1);
			if(!nullchk(Name, objLimit))
				file_corrupt("object", fileName); }
		if(Name[0] == '\0') Name = NULL;
		
		// get section type
		if(flags & IMAGE_SCN_LNK_COMDAT) {
			if(findSection(Name)) continue; }
		int type = sectTypeFromName(Name);
		if(type == 0) {	type = PeFile::Section::
			type(flags & ~IMAGE_SCN_LNK_COMDAT);
			if(type < 0) { fatal_error("object:Characteristics"
				" %X, \"%s\", \"%s\"", flags, Name, fileName); }
			printf("%s, %s\n", Name, fileName); continue;
		}

		// create new section
		Void pData = data ? objFile + data : Void(0);
		DWORD align = sect_align(flags);
		sectMapp[i] = addSection(fileName, Name, 
			pData, type, align, 0, size);
	}
	
	// read symbols
	Symbol** symMapp = xCalloc(nSymbols);
	for(int i = 0; i < nSymbols; i++)
	{
		// get symbol name
		char symName_[9];
		char* symName = NULL;
		if( objSym[i].Name1 != 0 ) {
			symName = symName_;
			strNcpy(symName, (char*)&objSym[i].Name1, 8);
		} else {
			symName = strTab + objSym[i].Name2;
			if(!nullchk(symName, objLimit))
				file_corrupt("object", fileName);
			if(symName[0] == '\0')
				symName = NULL;
		}
		
		// check StorageClass
		DWORD sclass = objSym[i].StorageClass;
		if(sclass == 6) sclass = 3; // I have not idea
		if((sclass != 2)&&(sclass != 3)&&(sclass != 105)){
			if((sclass == 103)||(sclass == 0))
				continue;
			fatal_error("object:StorageClass %d, \"%s\", \"%s\"\n",
				sclass, symName, fileName); 
		}
		
		// section type
		DWORD section = objSym[i].Section;
		Section* sectIndex;
		if(section == WORD(-2))
			file_bad("object", fileName);
		ei(section == WORD(-1))
			sectIndex = Type_Absolute;
		ei(section == 0)
			sectIndex = Type_Undefined;
		else {
			if(section > nSects)
				file_corrupt("object1", fileName);
			sectIndex = sectMapp[section-1];
			if(sectIndex == NULL) continue;
		}
		
		// weak symbol
		Symbol* weakSymb = 0;
		if(sclass == 105) {
			if(objSym[i].NumberOfAuxSymbols != 1)
				file_corrupt("object2", fileName);
			DWORD iWeakSymb = objSym[i+1].Name1;
			if(iWeakSymb >= i)
				file_bad("object: weak symbol", fileName);
			weakSymb = symMapp[iWeakSymb];
		}
		
		// check section symbol
		bool sectSymb = false;
		if(sclass == 3) {
			if(!objSym[i].Value	&& sectIndex->isReal()
			&& sectIndex->nameIs(symName)) { sectSymb = true; 
				if(sectTypeNormal(symName)) goto L1;
			} else { L1: symName = NULL; }
		}

SYMB_RETRY:
		symMapp[i] = addSymbol(
			symName, sectIndex, weakSymb, objSym[i].Value);
		if(symMapp[i] == NULL) { 
			if(sectSymb) { symName = 0; goto SYMB_RETRY; }
			Section* prevSect = findSymbol(symName)->section;
			fatal_error("object:duplicate symbol, %s\n"
				"defined in: %s;%s\nprevious in: %s;%s\n", 
				symName ? symName : "##NO NAME##",
				fileName, sectName(sectIndex), 
				sectFile(prevSect), sectName(prevSect));
		}
		
		if(sectSymb) sectIndex->symbol = symMapp[i];
		i += objSym[i].NumberOfAuxSymbols;
	}

	// read relocs
	for(int i = 0; i < nSects; i++)
	{
		if(sectMapp[i] == NULL) continue;
		auto& sect = *sectMapp[i];
		ObjRelocs* relocs = objFile + objSects[i].PointerToRelocations;
		DWORD nRelocs = objSects[i].NumberOfRelocations;
		if((relocs + nRelocs) > objLimit)
			file_corrupt("object3", fileName);
		
		for(auto& reloc : Range(relocs, nRelocs))
		{
			WORD type;
			
			// i386 relocation types
			if(!PeFILE::peFile.PE64) {
			switch(reloc.type) {
			case 0x06: type = Type_DIR32; break;
			case 0x07: type = Type_DIR32NB; break;
			case 0x14: type = Type_REL32; break;
			default: BAD_TYPE: FATAL_ERROR(
				"object: unsupported reloc, %d",
					reloc.type, fileName);	}

			// x64 relocation types
			} else { switch(reloc.type) {
			case 0x01: type = Type_DIR64; break;
			case 0x02: type = Type_DIR32; break;
			case 0x03: type = Type_DIR32NB; break;
			case 0x04: type = Type_REL32; break;
			default: goto BAD_TYPE; }}				
					
			// register reloc
			if((reloc.symbol >= nSymbols)
			||(int(symMapp[reloc.symbol]) < 0))
				file_corrupt("object4", fileName);
			sect.addReloc(type, reloc.offset,
				symMapp[reloc.symbol]);
		}
	}
}
