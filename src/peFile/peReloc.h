#pragma once

struct PeReloc : xVectorW<xVectorW<u16>>
{
	void Add(u32 rva); 
	bool Find(u32 rva);
	void Remove(u32 rva);
	void Remove(u32 rva, u32 length);
	void Move(u32 rva, u32 length, int delta);
	
	bool Load(byte* data, u32 size, bool PE64);
	u32 build_size(void);
	void build(byte* data, bool PE64);
};

#define PEFILE_ENUM_RELOCS(peFile, ...) \
	for(u32 _bi_ = 0; _bi_ < peFile.relocs.size; _bi_++) { \
	for(u32 rva : peFile.relocs.data[_bi_]) { \
		rva += _bi_<<12; __VA_ARGS__; }}
