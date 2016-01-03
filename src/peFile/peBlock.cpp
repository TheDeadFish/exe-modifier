
struct FreeBlock
{
	WORD type;
	WORD flags;
	int extent;
	int baseRva;
	int endRva;
	
	int length(void) {
		return endRva-baseRva; }
	int cost(void);
	float score(void);
	retpair<FreeBlock*,int*> getTempList(
		PeSection* sect, int& nBlocks);
	void mergeTempList(FreeBlock* tempList,
		int* indexList, int tmpCount);
};

int FreeBlock::cost(void)
{
	int extent = this->extent;
	if(flags & 1) extent = baseRva;
	return baseRva - alignDisk(extent);
}

float FreeBlock::score(void)
{
	int length = this->length();
	if(flags & 2) length = 4096;
	if(length < 1) length = 1;
	int cost = this->cost();
	if(cost < 0) cost = 0;
	return float(length) / cost;
}

retpair<FreeBlock*,int*> FreeBlock::getTempList(
	PeSection* sect, int& nBlocks)
{
	DWORD baseRva = 0;
	DWORD endRva = imageSize;
	if(sect == NULL) {
		FreeBlock* tmpBlocks = xMalloc(nBlocks);
		memcpyX(tmpBlocks, this, nBlocks*sizeof(*this));
		return retpair<FreeBlock*,int*>(tmpBlocks, 0);
	} else {
		DWORD baseRva = sect->baseRva();
		DWORD endRva = sect->endPage();
		int* indexList = NULL;
		int tmpCount = 0;
		for(int i = 0; i < nBlocks; i++)
		  if((this[i].baseRva >= baseRva)
		  &&(this[i].baseRva < endRva))
			xNextAlloc(indexList, tmpCount) = i;
		FreeBlock* tmpBlocks = xMalloc(tmpCount);
		for(int i = 0; i < tmpCount; i++) {
			tmpBlocks[i] = this[indexList[i]]; }
		nBlocks = tmpCount;
		return retpair<FreeBlock*,int*>(tmpBlocks, indexList);
	}
}

void FreeBlock::mergeTempList(FreeBlock* tempList,
	int* indexList, int tmpCount)
{
	if(indexList == NULL) {
  	  for(int i = 0; i < tmpCount; i++)
		this[i] = tempList[i]; }
	else {
	  for(int i = 0; i < tmpCount; i++)
		this[indexList[i]] = tempList[i]; }
	free(tempList); free(indexList); 
}

void allocBlocks(PeBlock* blocks, int nBlocks,
	FreeBlock* freeBlock, int freeCount, int type)
{
	if(blocks == NULL)
		return;
	for(auto& peBlock : Range(blocks, nBlocks))
	if(peBlock.type == type)
	{
		// choose best free block
		FreeBlock* bestBlock = 0;
		float bestScore = 0;
		for(auto& fBlock : Range(freeBlock, freeCount))
		{
			if(fBlock.type != type ) {
			  if(((fBlock.type != Type_Text)
			  ||(type != Type_RData))
			  &&(((fBlock.type != Type_Data)
			  ||(type != Type_Bss))))
				continue; }
				
			int allocBase = ALIGN(fBlock.baseRva, peBlock.align-1);
			if(((fBlock.endRva < (allocBase + peBlock.length))
			&&((fBlock.flags & 2) == 0))
			||(fBlock.score() <= bestScore))
				continue;
				
			bestBlock = &fBlock;
			bestScore = fBlock.score();
			if(type == Type_Bss)
				break;
		}
		
		// update block
		bestBlock->baseRva = ALIGN(bestBlock->baseRva, peBlock.align-1);
		peBlock.baseRva = bestBlock->baseRva;
		bestBlock->baseRva += peBlock.length;
		bestBlock->flags |= 1;
	}
}

void allocBlocks(PeBlock* blocks, int nBlocks)
{
	// prepare imports
	retpair<PeBlock*, int> impBlock(0);
	int iatBlockIndex, iatImpIndex;
	for(int i = 0; i < nImports; i++)
  	  if(imports[i].FirstThunk == 0) {
		impBlock = imports->getBlocks(
			nImports, &iatBlockIndex);
		iatImpIndex = i; break; }
	if(impBlock == NULL)
		freeList.unMark(Type_ImpDir);

	// complete and aquire freeList
	//getHeaderFree();
	int extndIndex = sectIdx(extndSection);
	for(int i = 0; i <= extndIndex; i++) {
		BYTE type = peSects[i].type() | i<<7;
		freeList.reMapp(peSects[i].basePtr(),
			peSects[i].vSize(), type);
		freeList.mark(peSects[i].endPtr(),
			peSects[i].remain(), type); 
	}
	int freeCount;
	auto freeLst = freeList.calcFree(freeCount);
	for(auto& freeBlock : Range(freeLst, freeCount)) {
		memset(freeBlock.basePtr, 0, freeBlock.length);
		freeBlock.type &= 127; }
	
	// convert freeList to FreeBlock
	FreeBlock* freeBlock = xMalloc(freeCount+2);
	for(int i = 0; i < freeCount; i++) {
		int baseRva = ptrToRva(freeLst[i].basePtr);
		int endRva = baseRva+freeLst[i].length;
		freeBlock[i].type = freeLst[i].type;
		freeBlock[i].baseRva = baseRva;
		freeBlock[i].endRva = endRva;
		freeBlock[i].flags = 1;
		for(int j = 0; j <= extndIndex; j++)
		if( ALIGN_PAGE(peSects[j].endRva()) == endRva ) {
			assert( freeBlock[i].type == peSects[j].type() );
			freeBlock[i].flags = 0;
			freeBlock[i].extent = peSects[j].baseRva() +
				peSects[j].extent(); }
	}

	// register expandable FreeBlock
	auto sectBlock = [&](int type) {
		auto& block = xNextAlloc(freeBlock, freeCount);
		block.type = type;
		block.flags = 3;
		block.baseRva = (&block)[-1].endRva;
		block.endRva = block.baseRva; };
	if(( extndSection->type() == Type_Data )
	&&( freeBlock[freeCount-1].cost() < 4096 ))
		freeBlock[freeCount-1].flags = 6;
	else sectBlock(Type_Data);
	sectBlock(Type_Text);

	// allocate import address table
	if(impBlock.a != NULL) 
	{
		int impOffset = iatImpIndex-iatBlockIndex;
		int tmpCount = freeCount;
		FreeBlock* tmpBlocks; int* indexList;
		getpair(tmpBlocks, indexList, freeBlock->getTempList(
			Import_UpdateDataDir(false), tmpCount));
		
		for(int i = iatBlockIndex; i < impBlock.b; i++) 
		{
			int iatBase = dataDir[IDE_IATABLE].VirtualAddress;
			int iatEnd = iatBase + dataDir[IDE_IATABLE].Size;
			PeBlock& peBlock = impBlock.a[i];
			FreeBlock* bestBlock = 0;
			int bestDiff = 0x7FFFFFFF;
			int bestBase, bestEnd;

			for(auto& fBlock : Range(tmpBlocks, tmpCount))
			{
				int curDiff, curBase, curEnd;
				if((fBlock.baseRva > iatBase) || (!iatBase)) {
					curBase = ALIGN(fBlock.baseRva, peBlock.align-1);
					curEnd = curBase + peBlock.length;
					if(curEnd > fBlock.endRva)
						continue;
					curDiff = curBase - iatEnd;
				} else {
					curEnd = fBlock.endRva & ~(peBlock.align-1);
					curBase = curEnd - peBlock.length;
					if(curBase < fBlock.baseRva)
						continue;
					curDiff = iatBase - curEnd;
				}
				assert(curDiff >= 0);
				if(bestDiff >= curDiff) {
					bestDiff = curDiff; bestBlock = &fBlock; 
					bestBase = curBase;	bestEnd = curEnd; }
			}
			if(bestBlock == NULL) {
				free(tmpBlocks); free(indexList);
				goto ALLOC_IMPORT_FAIL; }
			if(bestBlock->baseRva > iatBase) {
				bestBlock->baseRva = bestEnd; }
			else {
				bestBlock->endRva = bestBase; }
			imports[i+impOffset].FirstThunk = bestBase;
			Import_UpdateDataDir(false);
		}
		freeBlock->mergeTempList(tmpBlocks, 
			indexList, tmpCount);
		impBlock.b = iatBlockIndex;
	}
ALLOC_IMPORT_FAIL:
	
	// allocate blocks, .data & .bss
	allocBlocks(impBlock.a, impBlock.b, freeBlock, freeCount, Type_Data);
	allocBlocks(blocks, nBlocks, freeBlock, freeCount, Type_Data);
	allocBlocks(blocks, nBlocks, freeBlock, freeCount, Type_Bss);
	freeBlock[freeCount-1].baseRva = ALIGN_PAGE(freeBlock[freeCount-2].baseRva);
	freeBlock[freeCount-1].endRva = freeBlock[freeCount-1].baseRva;
	
	// allocate blocks, .rdata & .text
	allocBlocks(blocks, nBlocks, freeBlock, freeCount, Type_Text);	
	allocBlocks(blocks, nBlocks, freeBlock, freeCount, Type_RData);
	allocBlocks(impBlock.a, impBlock.b, freeBlock, freeCount, Type_RData);
	
	// extend real sections
	for(int i = 0; i < freeCount-2; i++)
	for(int j = 0; j <= extndIndex; j++)
	  if( ALIGN_PAGE(peSects[j].endRva()) == freeBlock[i].endRva )
		peResize(&peSects[j], freeBlock[i].baseRva-peSects[j].baseRva());
	FreeBlock* block = &freeBlock[freeCount-2];
	int baseRva = block[-1].endRva;
	int endRva = block[0].baseRva;
	if( block->flags & 4 )
		peResize(extndSection, endRva-baseRva);
	ei( endRva > baseRva )
		addSect(".data2", endRva-baseRva, true);
	block = &freeBlock[freeCount-1];
	baseRva = max(block[-1].endRva, ALIGN_PAGE(endRva));
	endRva = block[0].baseRva;
	if(endRva > baseRva)
		addSect(".text2", endRva-baseRva, false);
		
	// rebuild imports
	if(impBlock != NULL) 
	{	
		imports->rebuild(impBlock.a, impBlock.b, imageData, nImports);
		Import_UpdateDataDir(true);
		dataDir[IDE_IMPORT].VirtualAddress = impBlock.a->baseRva;
		dataDir[IDE_IMPORT].Size = impBlock.a->length;
	}
}
