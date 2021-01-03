#include "stdafx.h"
#include "peReloc.h"

bool PeReloc::Load(byte* buff, 
	u32 size, bool PE64)
{
	// determine last relocation block
	IMAGE_BASE_RELOCATION* relocPos = Void(buff);
	IMAGE_BASE_RELOCATION* relocEnd = Void(buff,size-8);
	u32 maxRva = 0; 
	while((relocPos < relocEnd) && relocPos->SizeOfBlock) {
		max_ref(maxRva, relocPos->VirtualAddress);
		PTRADD(relocPos, relocPos->SizeOfBlock); }
		
		
	// read the relocation blocks	
	xncalloc2((maxRva>>12)+1); relocPos = Void(buff);
	while((relocPos < relocEnd) && relocPos->SizeOfBlock) {
		auto& br = data[relocPos->VirtualAddress>>12]; int count = 
		(relocPos->SizeOfBlock-8)/2; br.xreserve(count); relocPos++;
		while(count--) { u32 wReloc = RDI(PW(relocPos));
			u32 type = wReloc / 4096; u32 val = wReloc & 4095;
			if(type != 0) { if(type != (PE64 ? 10 : 3))
				return false; br.ib() = val; }
		}
	}
	
	return true;
}

u32 PeReloc::build_size(void)
{
	u32 totalSize = 0;
	for(auto& br : *this) if(br.size) {
	totalSize += ALIGN4(br.size*2+8); }
	return totalSize;
}

void PeReloc::build(byte* buff, bool PE64)
{
	int type = (PE64 ? 0xA000 : 0x3000);
	IMAGE_BASE_RELOCATION* curPos = Void(buff);
	for(u32 bi = 0; bi < size; bi++) if(data[bi].size) {
		curPos->VirtualAddress = bi<<12; auto& br = data[bi]; 
		curPos->SizeOfBlock = ALIGN4(br.size*2+8); curPos++;
		qsort(br.data, br.size, [](const u16& a, const u16& b) {
			return a-b; }); for(u16 val : br) { WRI(PW(
			curPos), val | type); } if(br.size&1) PW(curPos)++; 
	}
}

bool PeReloc::Find(u32 rva)
{
	u32 block = rva>>12; rva &= 4095;
	if(block < size)
	for(auto val : data[block])
	if(val == rva) return true;
	return false;
}

void PeReloc::Add(u32 rva)
{
	if(Find(rva)) return;
	u32 block = rva>>12; rva &= 4095;
	xncalloc2(block+1);
	data[block].push_back(rva);
}

void PeReloc::Remove(u32 rva) {
	return Remove(rva, 1); }

void PeReloc::Remove(u32 rva, u32 len)
{
	u32 rvaEnd = rva+len;
	for(u32 bi = rva>>12; bi < size; bi++) {
	u32 base = bi<<12; if(base >= rvaEnd) break;
	for(u32 i = 0; i < data[bi].size; i++) {
	  if(inRng1(data[bi][i]+base, rva, rvaEnd))
	    data[bi][i] = data[bi][--data[bi].size]; }
	}
}

void PeReloc::Move(u32 rva, u32 length, int delta)
{
	u32 end = rva + length;
	u32 bi = rva>>12; u32 be = end>>12;
	if(delta > 0) std::swap(bi, be);
	while(1) { xArray<u16> tmp; tmp.init(data[bi].
		data, data[bi].size); data[bi].init();
		u32 base = bi<<12; for(u16 val : tmp) { 
		u32 rlc = val+base; if(inRng(rlc, rva, end)) 
			rlc += delta; this->Add(rlc); }
		if(bi == be) break; bi += delta > 0 ? -1 : 1;
	}
}
