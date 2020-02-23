#pragma once
#include "peHead.h"

struct PeMapImg
{
	IMAGE_NT_HEADERS64* inh;
	xarray<IMAGE_SECTION_HEADER> sects;
	xarray<PeDataDir> dataDir;
	xArray<byte> data;
	xArray<byte> symb;
	
	// data access
	IMAGE_OPTIONAL_HEADER64* ioh() { 
		return &inh->OptionalHeader; }
		
	// helper functions
	DWORD sectAlign(DWORD v) { 
		return peHead_sectAlign(inh, v); }
	DWORD fileAlign(DWORD v) { 
		return peHead_fileAlign(inh, v); }
		
	bool PE64() { return peHead64(inh); }
	u64 imageBase() { return peHead_imageBase(inh); }
	
	int load(cch* file);
	
	PeMapImg() { ZINIT; }
};
