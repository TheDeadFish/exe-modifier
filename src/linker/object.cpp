
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

struct DRective { 
	char* str_;
	cstr name; cstr arg[2];
	DRective(char* str) : str_(str) {}
	bool parse(void);

//private:
	static bool isWsOrNull(int ch) { return (u8(ch) <= ' '); }
	static char* strend(char* str, char ch) {
		while(!isWsOrNull(*str) && (*str != ',')) str++;
		return str; }
};

bool DRective::parse(void)
{
	memfill2(name, arg, 0);
	char* str = str_;

	// get the name
	while(*str == ' ') str++;
	if(!*str){ str_ = str; return 0; }
	if(*str++ != '-') return 0;
	str = strchr((name.data = str), ':');
	if(!str) return 0; name.setend(str);
		
	// parse the argument
	for(int i = 0; i < 2; i++) {
		if(*++str == '"') {
			str = strchr(arg[i].data = ++str, '\"');
			if(!str) return 0; arg[i].setend(str++);
		} else {
			str = strend(arg[i].data = str, ',');
			if(!str) return 0; arg[i].setend(str); }
		if(*str != ',') {
			if(isWsOrNull(*str)) {
				str_ = str; return 1; }
			break; }
	}
	return 0;
}

void object_drective(
	const char* fileName, char* str)
{
	DRective dr(str);
	int value;
	
	while(dr.parse()) {
	
		// parse aligncomm
		if(!dr.name.cmp("aligncomm")) {
			if(!dr.arg[1].parseInt(&value)) goto ERR;
			Symbol* symb = findSymbol(dr.arg[0]);
			if(!symb) goto ERR; 
			Section* sect = symb->section;
			if((!sect->isReal())||(symb->value))
				continue;
			max_ref(sect->align, 1<<value);		
		}
	}

	// print error message
	if(*dr.str_) {
		error_msg(".drectve bad parse: %s\n", dr.str_);
		if(0) { ERR: error_msg(".drectve bad args: %.*s, %.*s, %.*s\n",
				dr.name.prn(), dr.arg[0].prn(), dr.arg[1].prn()); }
		file_bad("object:.drectve", fileName);
	}
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
	cstr drective = {};
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
		}

		// create new section
		Void pData = data ? objFile + data : Void(0);
		if(Name && !strcmp(Name, ".drectve")) {
			drective = {pData, size}; continue; }
		DWORD align = sect_align(flags);
		sectMapp[i] = addSection(fileName, Name, 
			pData, type, align, size);
	}
	
	// read symbols
	Symbol** symMapp = xCalloc(nSymbols);
	for(int i = 0; i < nSymbols; 
		i += objSym[i].NumberOfAuxSymbols+1)
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
			if(isNeg(objSym[i].Value)) continue;
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
		DWORD symbFlags = 0;
		if(sclass == 3) {	symbFlags = SYMBFLAG_STATIC;
			if(!objSym[i].Value	&& sectIndex->isReal()
			&& sectIndex->nameIs(symName)) {
				symbFlags = SYMBFLAG_SECTION;
				if(sectTypeNormal(symName))
					symbFlags |= SYMBFLAG_STATIC;
			}
		}

		// create the symbol
		symMapp[i] = addSymbol(
			symName, sectIndex, weakSymb, objSym[i].Value, symbFlags);
		if(symMapp[i] == NULL) {
			Section* prevSect = findSymbol(symName)->section;
			fatal_error("object:duplicate symbol, %s\n"
				"defined in: %s;%s\nprevious in: %s;%s\n", 
				symName ? symName : "##NO NAME##",
				fileName, sectName(sectIndex), 
				sectFile(prevSect), sectName(prevSect));
		}
		
		// initialize merged section
		if(symbFlags & SYMBFLAG_SECTION) {
			sectIndex->symbol = symMapp[i];
			if(!sectTypeFromName(sectIndex->name))
				mergeSect_init(sectIndex); 	}
				
		// comdata section 
		if((sclass == 2)&&(objSym[i].Value)
		&&(symMapp[i]->section == Type_Undefined)) {
			int type = sectTypeFromName(".bss");
			symMapp[i]->section = addSection(fileName, 
				symName, 0, type, 0, objSym[i].Value);
			symMapp[i]->value = 0;
		}

		i += objSym[i].NumberOfAuxSymbols;
	}
	
	if(drective) object_drective(fileName, xstr(drective.xdup()));

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
			if(!x64Mode()) {
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
