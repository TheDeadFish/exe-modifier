
DWORD* relocs; DWORD nRelocs;

void Relocs_Add(int rva)
{
	if(Reloc_Find(rva))
		return;
	if((nRelocs & 4095) == 0)
		relocs = xrealloc(relocs, (nRelocs + 4096) * 4);
	relocs[nRelocs] = rva;
	nRelocs++;
}

bool Reloc_Find(int rva)
{
	for(int i = 0; i < nRelocs; i++)
	  if(relocs[i] == rva)
		return true;
	return false;
}

int Relocs_Remove(int rva)
{	return Relocs_Remove(rva, 1);	}
int Relocs_Remove(int rva, int length)
{
	int foundReloc = 0;
	int rvaEnd = rva + length;
	for(int i = 0; i < nRelocs; i++)
	  if((relocs[i] >= rva) && (relocs[i] < rvaEnd))
	{
		relocs[i] = relocs[--nRelocs];
		foundReloc += 1;
	}
	return foundReloc;
}

int Relocs_Load(void)
{
	if(relocSection == NULL) return 0;
	Void relocPos = relocSection->basePtr();
	Void relocEnd = relocSection->endPtr();
	WORD*& reloc = relocPos.ptr<WORD>();
	relocs = xMalloc(ALIGN_PAGE(
		((int(relocEnd - relocPos))>>1)));
	
	while((relocEnd - relocPos) > 8)
	{
		IMAGE_BASE_RELOCATION* baseReloc = 
		relocPos.ptr<IMAGE_BASE_RELOCATION>()++;
		size_t addr = baseReloc->VirtualAddress;
		if( baseReloc->SizeOfBlock == 0 ) break;
		int count = (baseReloc->SizeOfBlock - 8) / 2;

		while(count--)
		{
			WORD wReloc = *reloc++;
			int type = wReloc / 4096;
			int offset = wReloc & 4095;
			if(type != 0)
			{
				if(type != 3) return 1;
				relocs[nRelocs++] = addr+offset;
			}
		}
	}
	return 0;
}

int Relocs_Save(void)
{
	if(relocSection == 0) 
		return 0;
	qsort( relocs, nRelocs, [](const DWORD& a,
		const DWORD& b) { return int(a - b); });
	
	// calculate space for relocations
	int totalSize = 0;
	int lastRva = -1;
	for(int i = 0; i < nRelocs; i++)
	{
		int rva = relocs[i] & ~4095;
		if(rva != lastRva)
		{
			lastRva = rva;
			totalSize = ALIGN4(totalSize);
			totalSize += 8;
		}
		totalSize += 2;
	}
	peResize( relocSection, totalSize );
	dataDir[IDE_BASERELOC].Size = totalSize;
	
	// save relocations
	Void relocPos = relocSection->basePtr();
	Void basey = relocPos;
	IMAGE_BASE_RELOCATION* baseReloc = NULL;
	lastRva = -1;
	for(int i = 0; i < nRelocs; i++)
	{
		int rva = relocs[i] & ~4095;
		if(rva != lastRva)	
		{
			lastRva = rva;
			if(baseReloc && (baseReloc->SizeOfBlock & 2))
			{
				baseReloc->SizeOfBlock += 2;
				*relocPos.ptr<WORD>()++ = 0;
			}
			baseReloc = relocPos.ptr<IMAGE_BASE_RELOCATION>()++;
			baseReloc->VirtualAddress = rva;
			baseReloc->SizeOfBlock = 8;
		}
		baseReloc->SizeOfBlock += 2;
		*relocPos.ptr<WORD>()++ = 0x3000 | (relocs[i] & 4095);
	}
	return 0;
}

int Relocs_Rebase(int offset)
{
	for(uint reloc : Range(relocs, nRelocs))
	{
		if(reloc >= imageSize-4)
			return 1;
		(imageData+reloc).dword() += offset;
	}
	return 0;
}

void Relocs_Free(void)
{
	free_ref(relocs);
	nRelocs = 0;
}

void Relocs_Report(DWORD* oldRelocs, int nOldRelocs)
{
	int oldPos = 0;
	int newPos = 0;	
	while ((oldPos < nOldRelocs)
		&&(newPos < nRelocs))
	{
		if(oldRelocs[oldPos] == relocs[newPos]) {
			oldPos++; newPos++; continue; }
		if(oldRelocs[oldPos] < relocs[newPos]) {
			int curOldVal = oldRelocs[oldPos++];
			printf("-, %X, %X\n", curOldVal, rvaToAddr(curOldVal));
		} else {
			int curNewVal = relocs[newPos++];
			printf("+, %X, %X\n", curNewVal, rvaToAddr(curNewVal));
		}
	}
}
