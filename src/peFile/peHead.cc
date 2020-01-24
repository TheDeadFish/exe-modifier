#include "stdafx.h"
#include "peHead.h"

void PeOptHead_::update(IMAGE_SECTION_HEADER* ish)
{
	SizeOfImage += sectAlign(ish->Misc.VirtualSize);
	u32 vSzFA = fileAlign(ish->Misc.VirtualSize);
	if(ish->Characteristics & IMAGE_SCN_CNT_CODE) { SizeOfCode += vSzFA; 
		if(!BaseOfCode) BaseOfCode = ish->VirtualAddress;		
	} ei(ish->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
		SizeOfInitializedData += vSzFA; goto L1;
	} ei(ish->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
		SizeOfUninitializedData += vSzFA; L1:  
		if(!PE64() && !BaseOfData32) BaseOfData32 = ish->VirtualAddress;	
	}
}
