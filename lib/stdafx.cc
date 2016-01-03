#include "stdshit.cc"

int strifnd(const char* str, const char** tab, int len)
{
	for(int i = 0; i < len; i++)
	  if(!stricmp(str, tab[i]))
 	    return i+1;
	return 0;
}
