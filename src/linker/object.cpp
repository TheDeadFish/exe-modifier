
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
	DWORD* sectMapp = xMalloc(nSects);
	memset(sectMapp, -1, nSects*4);
	for(int i = 0; i < nSects; i++)
	{
		// get section name
		char* Name = (char*)objSects[i].Name;
		if(Name[0] == '/') { Name = strTab + atoi(Name+1);
			if(!nullchk(Name, objLimit))
				file_corrupt("object", fileName); }
		if(Name[0] == '\0') Name = NULL;
		int type = sectTypeFromName(Name); 
		if(type < 0) { continue; }
		
		// register section
		if( objSects[i].VirtualAddress != 0 )
			file_bad("object", fileName);
		DWORD sectSize = objSects[i].SizeOfRawData;
		if(!sectSize && (type!=4)) { continue; }
		Void sectData = NULL;
		if( objSects[i].PointerToRawData != 0 )	{
			sectData = objFile + objSects[i].PointerToRawData;
			if( (sectData + sectSize) > objLimit )
				file_corrupt("object", fileName); 
		}
		int align = (objSects[i].Characteristics>>20)&15;
		if(align != 0) align = 1 << (align-1);
		sectMapp[i] = addSection(fileName, Name, 
			sectData, type, align, 0, sectSize);
	}
	
	// read symbols
	DWORD* symMapp = xMalloc(nSymbols);
	memset(symMapp, -1, nSymbols*4);
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
		if(sclass == 6) sclass = 2; // I have not idea
		if((sclass != 2)&&(sclass != 3)&&(sclass != 105)){
			if((sclass == 103)||(sclass == 0))
				continue;
			fatal_error("object:StorageClass %d, \"%s\", \"%s\"\n",
				sclass, symName, fileName); 
		}
		
		// section type
		DWORD section = objSym[i].Section;
		DWORD sectIndex;
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
			if(isNeg(sectIndex)) continue;
		}
		
		// weak symbol
		DWORD weakSymb = -1;
		if(sclass == 105) {
			if(objSym[i].NumberOfAuxSymbols != 1)
				file_corrupt("object2", fileName);
			weakSymb = objSym[i+1].Name1;
			if(weakSymb >= i)
				file_bad("object: weak symbol", fileName);
			weakSymb = symMapp[weakSymb];
		}

		symMapp[i] = addSymbol((sclass == 3) ? NULL :
			symName, sectIndex, weakSymb, objSym[i].Value);
		if(isNeg(symMapp[i])) {
			FATAL_ERROR("object:duplicate symbol, %s",
				symName ? symName : "##NO NAME##", fileName);
		}
		i += objSym[i].NumberOfAuxSymbols;
	}

	// read relocs
	for(int i = 0; i < nSects; i++)
	{
		if(isNeg(sectMapp[i])) continue;
		auto& sect = sections[sectMapp[i]];
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
