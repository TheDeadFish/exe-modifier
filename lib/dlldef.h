#pragma once

struct DefFile
{
	DEF_RETPAIR(LineInfo_t, 
		int, line, int, row);
	LineInfo_t load(char* pos);
	
	struct Export {
		char* name, *frwd; int ord; 
		bool NoName, Private, Data;	
	};
	
	char* name; u64 base;
	xArray<Export> expLst;
};
