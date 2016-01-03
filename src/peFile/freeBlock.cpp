#include "freeBlock.h"

void FreeList::mark(Void basePtr, int length, BYTE type)
{
	if(length <= 0) return;
	Void endPtr = ALIGN4(size_t(basePtr+length));
	if((basePtr < freeBasePtr)
	||(endPtr > freeEndPtr)) {
		
		// calculate new size
		if(freeBytes == 0) {
			freeBasePtr = basePtr;
			freeEndPtr = basePtr; }
		int newBasePtr = min(freeBasePtr, basePtr);
		int newEndPtr = max(freeEndPtr, endPtr);
		int oldSize = freeEndPtr-freeBasePtr;
		int reqSize = newEndPtr-newBasePtr;

		// reallocate
		if(reqSize > oldSize) {
			int newSize = max(reqSize, oldSize*2);
			newEndPtr = newBasePtr + newSize;
			xrealloc(freeBytes, newSize);
			int extIdx = freeEndPtr-newBasePtr;
			int extLen = newEndPtr-freeEndPtr;
			memset(freeBytes+extIdx, -1, extLen);}
			
		// shuffle
		if( freeBasePtr > newBasePtr ) {
			int preIdx = freeBasePtr-newBasePtr;
			memmove(freeBytes+preIdx, freeBytes, oldSize);
			memset(freeBytes, -1, preIdx); }
		freeBasePtr = newBasePtr;
		freeEndPtr = newEndPtr;
	}
	
	// mark free space
	int baseIdx = basePtr-freeBasePtr;
	memset(freeBytes+baseIdx, type, length);
	int testLen = endPtr-(basePtr+length);
	for(int i = 0; i < testLen; i++)
		freeBytes[baseIdx+length+i] = -2;
}

retpair<BYTE*, int> FreeList::BaseLen(
	Void basePtr, int len) 
{
	Void endPtr = min_max(basePtr+len, 
		freeBasePtr, freeEndPtr);
	basePtr = min_max(basePtr, 
		freeBasePtr, freeEndPtr);
	return retpair<BYTE*, int> 
		(basePtr + PTRDIFF(freeBytes, freeBasePtr),
		 endPtr-basePtr);
}

void FreeList::reMapp(Void basePtr, int len, BYTE type)
{
	auto baseLen = BaseLen(basePtr, len);
	for(int i = 0; i < baseLen.b; i++)
	  if(baseLen.a[i] < 0xFE)
		baseLen.a[i] = type;
}

void FreeList::unMark(Void basePtr, int len)
{
	auto baseLen = BaseLen(basePtr, len);
	memset(baseLen.a, 0, baseLen.b);
}

void FreeList::unMark(BYTE type)
{
	int length = freeEndPtr-freeBasePtr;
	for(int i = 0; i < length; i++)
	  if(freeBytes[i] == type)
		freeBytes[i] = 0xFF;
}

FreeList::Block* FreeList::calcFree(int& blockCount)
{
	// fill in the gaps
	int length = freeEndPtr-freeBasePtr;
	int runLen = -1; BYTE lastType;
	for(int i = 1; i < length; i++) 
	{
		if( freeBytes[i] == 0xFF )
			runLen = -1;
		ei( freeBytes[i] == 0xFE ) {
			if( runLen >= 0 )
				runLen += 1;
		} else {
			if(( runLen > 0 )
			&&((( runLen == 1 ) && !(size_t(freeBasePtr+i) & 1))
			||(( runLen <= 3 ) && !(size_t(freeBasePtr+i) & 3))))
				for(int j = 1; j <= runLen; j++)
					freeBytes[i-j] = lastType;
			runLen = 0;	lastType = freeBytes[i];
		}
	}
	
	// generate list
	blockCount = 0;
	Block* blocks = 0;
	Block* curBlock = 0;
	for(int i = 0; i < length; i++)
	{
		if(freeBytes[i] >= 0xFE) {
			curBlock = 0;
			continue; 	}
		if((curBlock == NULL)
		||(freeBytes[i] != lastType)) {
			curBlock = &xNextAlloc(blocks, blockCount);
			curBlock->basePtr = freeBasePtr+i;
			curBlock->length = 0; 
			curBlock->type = freeBytes[i];
			lastType = freeBytes[i]; }
		curBlock->length += 1;
	}
	return blocks;
}
