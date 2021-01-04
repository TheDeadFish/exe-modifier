#include "stdafx.h"
#include "peExcept.h"

bool PeExcept::Load(byte* data, u32 size, u32 rva)
{
	funcs.init((RtFunc*)data, size / sizeof(RtFunc));
	for(auto& fn : funcs) { if(fn.addr & 1) {
			u32 idx = (fn.addr-rva) / sizeof(RtFunc);
			if(idx >= funcs.len) return false;
			fn.addr = idx | INT_MIN; }
	} return true;
}

void PeExcept::Rebase(u32 rva)
{
	rva += 1;
	for(auto& fn : funcs) {
		if(isNeg(fn.addr))
			fn.addr = (fn.addr*12)+rva;
	}
}

PeExcept::RtFunc* PeExcept::find(u32 rva, u32 len)
{
	u32 end = (rva+len)-1;
	for(auto& rtf : funcs) {
		if((rtf.start <= rva) &&(rtf.end >= end))
			if(rtf.addr) return &rtf; break; }
	return NULL;
}

void PeExcept::kill(u32 rva, u32 len)
{
	RtFunc* prtf = find(rva, len);
	if(prtf) prtf->addr = 0;
}

