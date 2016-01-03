#ifndef _FREEBLOCK_H_
#define _FREEBLOCK_H_

struct FreeList
{
	FreeList() { ZINIT; }
	~FreeList() { free(freeBytes); }
	struct Block {
		Void basePtr;
		int length; 
		BYTE type;	};
	void mark(Void basePtr, int length,	BYTE type);
	void markStr(Void basePtr, int length, BYTE type);
	void reMapp(Void basePtr, int length, BYTE type);
	void unMark(Void basePtr, int length);
	void unMark(BYTE type);
	Block* calcFree(int& blockCount);
	
	Void freeBasePtr;
	Void freeEndPtr;
	byte* freeBytes;
private:
	retpair<BYTE*, int> BaseLen(
		Void basePtr, int length);
};

#endif
