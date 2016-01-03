
bool rebaseRsrc( int offset, 
	IMAGE_RESOURCE_DIRECTORY* rsrcDir )
{
	// peform bounds check
	IMAGE_RESOURCE_DIRECTORY_ENTRY* entry = Void(rsrcDir+1);
	if(checkPtr(entry, 0) == false)
		return false;
	int nNmRsrc = rsrcDir->NumberOfNamedEntries;
	int nIdRsrc = rsrcDir->NumberOfIdEntries;
	if(checkPtr(entry, nNmRsrc+nIdRsrc) == false)
		return false;

	for(auto& ent : Range(entry, nNmRsrc+nIdRsrc))
	{
		int rva = rsrcSection->baseRva() +
			ent.OffsetToDirectory;
		if( entry->DataIsDirectory )
			rebaseRsrc(offset, imageData+rva);
		else {
			if(checkRva(rva) == false)
				return false;
			(imageData+rva).dword() += offset;
		}
	}
	return true;
}

void peResize(PeSection* sect, int newSize)
{
	// expand into reserved space?
	assert(sect != NULL);
	int vSizePage = ALIGN_PAGE(sect->vSize());
	if(sect->vSize() < newSize)
		sect->vSize() = newSize;
	if(vSizePage >= newSize)
		return;
	
	// expand section
	assert(sect >= extndSection);
	int newSizePage = ALIGN_PAGE(newSize);
	int delta = newSizePage-vSizePage;
	PTRADD(sect, peRealloc(delta));
	memmove(sect->basePtr()+newSizePage, 
		sect->basePtr()+vSizePage, 
		imageSize-sect->baseRva()-newSizePage);
	memset(sect->basePtr()+vSizePage, 0, delta);

	// peform fixups
	int index = sect-peSects;
	for(int i = index+1; i < nSections(); i++) {
		peSects[i].baseRva() += delta;
		peSects[i].basePtr() += delta; 
	}
	if(relocSection > sect)
		dataDir[IDE_BASERELOC].VirtualAddress += delta;
	if(rsrcSection > sect) {
		dataDir[IDE_RESOURCE].VirtualAddress += delta;
		if(!rebaseRsrc(delta, rsrcSection->basePtr()))
			fatalError("resource section corrupt"); 
	}
}

PeSection* addSect(
	const char* name, int size, bool isData) 
{
	// reserve space
	dataDir[IDE_BOUNDIMP].VirtualAddress = 0;
	dataDir[IDE_BOUNDIMP].Size = 0;
	int endIndex = 0;
	if(rsrcSection) { rsrcSection++; endIndex--; }
	if(relocSection) { relocSection++; endIndex--; }
	int section = nSections() + endIndex;
	PeSection* sect = &peSects[section];
	memmove(sect+1, sect, -sizeof(PeSection)*endIndex);
	nSections() += 1;
	
	// create section
	strcpy((char*)sect->Name, name);
	sect->vSize() = 0;
	sect->baseRva() = ALIGN_PAGE(sect[-1].endRva());
	sect->basePtr() = rvaToPtr(sect->baseRva());
	sect->Characteristics = isData ?
		0xC0000040 : 0x60000020;
	peResize(sect, size);
	return &peSects[section];
}
