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


// debug timer
static u64 qpc() { LARGE_INTEGER li; QueryPerformanceCounter(&li); return li.QuadPart; }
static u64 qpf() { LARGE_INTEGER li; QueryPerformanceFrequency(&li); return li.QuadPart; }
void DebugTimer::qpc_add(u64* p) { u64 tmp = qpc(); ARGFIX(p); *p += tmp; }
void DebugTimer::qpc_sub(u64* p) { u64 tmp = qpc(); ARGFIX(p); *p -= tmp; }
int DebugTimer::getms() { return total*1000 / qpf(); }
void DebugTimer::print() { printf("%d\n", getms()); }
