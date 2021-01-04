#pragma once

struct PeExcept
{
	struct RtFunc {
		DWORD start, end, addr; };
	xarray<RtFunc> funcs;
	
	bool Load(byte* data, u32 size, u32 rva);
	void Rebase(u32 rva);
	
	
	
	RtFunc* find(u32 rva, u32 len);
	void kill(u32 rva, u32 len);
};
