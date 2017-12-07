//#include "stdshit.cc"
//#include "c-parse.cc"
#include "stdshit.h"


int strifnd(const char* str, const char** tab, int len)
{
	for(int i = 0; i < len; i++)
	  if(!stricmp(str, tab[i]))
 	    return i+1;
	return 0;
}

SHITCALL cstr tempName(cch* prefix)
{
	// prepare prefix
	if(!prefix) prefix = "dft";
	WCHAR pfx[4]; 
	for(int i = 0; i < 4; i++)
		pfx[i] = prefix[i];
	
	// get tempory name
	WCHAR buff[MAX_PATH];
	GetTempPathW(MAX_PATH, buff);	
	GetTempFileNameW(buff, pfx, 0, buff);
	return utf816_dup(buff);
}
