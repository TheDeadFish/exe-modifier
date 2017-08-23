#include "stdafx.h"
#include "peImport.h"

static inline
void* xarray_insert(void* xa, void* ins, void* end, size_t sz) {
	size_t len = PTRDIFF(end,ins); ins -= RT(xa); xnxalloc2(
	xa, sz); ins += RT(xa); memmove(ins+sz, ins, len); return ins; }
	
bool FreeLst2_t::merge(FreeLst2_t& that, int sz)
{
	FreeLst2_t* a = this; FreeLst2_t* b = &that; 
	if((sz>8) && (a->iSect != b->iSect)) return false;
	if(a->rva >= b->rva) swapReg(a, b);
	u32 ext = (sz>8) ? ALIGN(a->end, b->align-1) : a->end;
	if(ext < b->rva) return false; ARGFIX(a->rva);
	if(a->end > b->end) b = a; rva = a->rva; end = b->end;
	if(sz>8) align = b->align; return true;
}

void FreeLst2_t::mark(void* xa, u32 sz,
	u32 rva, u32 end, u32 align, u32 iSect)
{
	FreeLst2_t tmp = {rva, end, align, iSect};
	auto& lst = *(xarray<FreeLst2_t>*)xa;
	
	// locate insertion point
	FreeLst2_t* endPos = Void(lst.data,lst.size*sz);
	FreeLst2_t* insPos = lst.data; while((insPos < endPos)
	  &&(((sz>8)&&(insPos->iSect < tmp.iSect))
	  ||(insPos->rva < tmp.rva))) PTRADD(insPos, sz);
	  
	// merge/insert tmp
	if(insPos > lst.data) { 
		FreeLst2_t* tmpPos = Void(insPos, -sz);
		if(tmpPos->merge(tmp, sz)) {
			insPos = tmpPos; goto MERGE_LOOP; }
	} if(insPos < endPos) {
		if(insPos->merge(tmp, sz)) goto MERGE_LOOP;
	} memcpy(xarray_insert(&lst, 
		insPos, endPos, sz), &tmp, sz);

	// merge loop
	if(0) { MERGE_LOOP: FreeLst2_t* mrgPos = Void(insPos, sz);
		for(;mrgPos < endPos; PTRADD(mrgPos, sz)) { 
		if(!insPos->merge(*mrgPos, sz)) break; lst.size--;}
		PTRADD(insPos, sz); if(insPos != mrgPos) memcpy(
			insPos, mrgPos, PTRDIFF(endPos,mrgPos));
	}
}

Void FreeLst::mark(u32 rva, u32 len, u32 align) {
	return mark(rva, len,  align, true); }

Void FreeLst::mark(u32 rva, u32 len, u32 align, bool doMark)
{
	auto* sect = peFile->rvaToSect(rva, len); 
	if(!sect) return NULL; rva -= sect->baseRva; 
	FreeLst2_t::mark(this, sizeof(FreeLst2_t), 
		rva, rva+len, align, sect-peFile->sects);
	return sect->data + rva;
}

struct FreeSect
{
	struct Sect : xArray<FreeLst_t> {
		WORD type; WORD iSect; 
		u32 extent; u32 endRva;
		void mark(u32 rva, u32 end) { FreeLst2_t::
			mark(this, 8, rva, end, 0, 0); }
	}; xArray<Sect> sects;
	PeFile& peFile;
	
	FreeSect(PeFile& peFile);
	void add(FreeLst& freeLst);
	void finalize();
	
	//void mark(FreeLst2_t& 
	
	void allocSects(void);
	bool allocBlocks(xarray<PeBlock> blocks);
	bool resolveBlocks(xarray<PeBlock> blocks);
};

FreeSect::FreeSect(PeFile& pef) : peFile(pef)
{
	int extIdx = pef.iSect2(pef.extendSect);
	sects.xcalloc(extIdx+3);
	for(int i = 0; i <= extIdx; i++) {
		sects[i].type = pef.sects[i].type();
		sects[i].endRva = pef.sects[i].len;
		sects[i].extent = pef.sects[i].extent(pef);
	} sects[extIdx+1].type = PeSecTyp::Text|PeSecTyp::NoPage;
	sects[extIdx+2].type = PeSecTyp::Data|PeSecTyp::NoPage;
}

void FreeSect::add(FreeLst& freeLst)
{
	for(auto& b : freeLst) {
		sects[b.iSect].mark(b.rva, b.end); }
}

void FreeSect::finalize() 
{
	for(int i = 0; i < sects.len; i++) {
		u32 end = 0x7FFFFFFF;
		if(i < (sects.len-3)) end = peFile.
			sectAlign(sects[i].endRva);
		sects[i].mark(sects[i].endRva, end); 
	}
}

bool FreeSect::allocBlocks(xarray<PeBlock> blocks)
{
	for(auto& block : blocks) 
	{
		if(block.peSect != 0) continue;
		FreeLst_t* bestSlot; Sect* bestSect;
		u32 bestEnd, bestCost = -1;
	
	for(auto& sect : sects) 
	{
		// check section type
		if((isNeg(sect.type))||((sect.type & 
		block.type) != block.type)) continue;
		
		// locate free slot
		FreeLst_t* slot; u32 rva, end;
		for(auto& slot1 : sect) { rva = ALIGN(slot1.rva, block.align-1); 
		if((block.type & PeSecTyp::Intd)&&(rva > sect.extent))
		goto CONT; end = rva+block.length; if(end <= slot1.end) { 
		slot = &slot1; goto FOUND; }} CONT: continue; FOUND:;
		
		// calculate slot score
		u32 tmp = sect.type & ~block.type;
		u32 cost = !!(tmp & PeSecTyp::Exec);
		cost += !!(tmp & (PeSecTyp::Write|PeSecTyp::Intd))*2;
		cost += !!(tmp & (PeSecTyp::NoDisc|PeSecTyp::NoPage))*4;
		if(end > sect.endRva) cost += 8;
		
		
		//printf("## %X, %X, %X, %X\n", (&sect-sects)+1, cost, block.type,  sect.type);
		
		
		if(bestCost>cost) {
			bestCost = cost; bestEnd = end;
			bestSlot = slot; bestSect = &sect; 
			if(cost == 0) break; }
	}
	
		// apply the block
		if(!isNeg(bestCost)) { 
			block.peSect = (bestSect-sects)+1; 
			bestSlot->rva = bestEnd; 
			block.baseRva = bestEnd-block.length;
			max_ref(bestSect->endRva, bestEnd);
			
			if((block.type & PeSecTyp::Intd)
			&&(bestSect->extent < bestEnd)) { bestSect->
				extent = peFile.fileAlign(bestEnd); }
			
			
			//printf("!! %X, %X, %X, %X\n", block.peSect, block.baseRva, 
			//	block.length, block.type);
			
			
			
		}
	}
}

bool FreeSect::resolveBlocks(xarray<PeBlock> blocks)
{
	for(auto& block : blocks) 
	{
		if(block.peSect == 0) return false;
		//printf("%d, %d, %d, ", block.peSect, block.lnSect, block.length);
		//printf("%d\n", sects[block.peSect-1].iSect);
		
		
		
		auto& sect = peFile.sects[
			sects[block.peSect-1].iSect];
		block.data = sect.data+block.baseRva;
		block.baseRva += sect.baseRva;
	}
	
	return true;
}

void FreeSect::allocSects()
{
	// allocate text2 & data2
	int extIdx = peFile.iSect2(peFile.extendSect);
	if(sects[extIdx+1].endRva) {
		peFile.sectCreate2(".text2", sects[extIdx+1].type); }
	if(sects[extIdx+2].endRva) {
		peFile.sectCreate2(".data2", sects[extIdx+2].type); }
		
	// resize sections
	int iSect = 0; 
	for(auto& sect : sects) { 
		sect.iSect = iSect; if(sect.endRva) { 
		if(peFile.sects[iSect].len < sect.endRva) 
			peFile.sectResize(peFile.sects+iSect, sect.endRva);
		iSect++; }
	}
}

int PeBlock::order() const 
{
	int val = 0x1000-align;
	if(!(type & PeSecTyp::Exec)) { val |= 
	(type & PeSecTyp::Write) ? 0x1000 : 0x2000; }
	if(!(type & PeSecTyp::Intd)) val |= 0x4000;
	return val;
}

int PeBlock::cmpFn(const PeBlock& a, const PeBlock& b)
{
	int diff = a.order()-b.order();
	if(!diff) diff = a.lnSect-b.lnSect;
	return diff;
}

void allocBlocks(xarray<PeBlock> blocks, PeFile& 
	peFile, PeImport* peImp, FreeLst* freeLst)
{
	FreeSect freeSect(peFile);
	
	// rebuild imports
	xArray<PeBlock> impBlock;
	if(peImp && peImp->mustRebuild()) {
		impBlock.init(peImp->getBlocks());
		freeSect.add(peImp->freeLst);
		freeSect.allocBlocks(impBlock);
	}
	
	// sort the blocks
	qsort(blocks.data, blocks.size, PeBlock::cmpFn);
	PeBlock* rdatPos = blocks;
	for(auto& block : blocks) { rdatPos = &block;
		if(((block.type & 7) <= 4)) break; }
		
	// perform the allocations
	freeSect.finalize();
	freeSect.allocBlocks({blocks.data, rdatPos});
	freeSect.allocBlocks(impBlock);
	freeSect.allocBlocks({rdatPos, blocks.end()});
	
	// resolve sections
	freeSect.allocSects();
	freeSect.resolveBlocks(blocks);
	freeSect.resolveBlocks(impBlock);
	peImp->build(impBlock);
}
