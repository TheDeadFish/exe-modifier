#include "waveout.h"
#include <math.h>
#define BUILDING_STATIC
#include "xmp.h"
const char progName[] = "test";
#include "music.cpp"

bool callback(xmp_context ctx, short* buffer, int length)
{
	xmp_play_buffer(ctx, buffer, length*4, 0);
	return true;
}

extern "C"
void RawEntryPoint(void);
extern "C"
void HookEntryPoint(void)
{
	xmp_context ctx = xmp_create_context();
	xmp_load_module_from_memory(ctx, (void*)musicData, sizeof(musicData));
	xmp_start_player(ctx, 48000, 0);
	
	WaveOut wo;
	wo.callBack = MakeDelegate2(ctx, callback);
	if(wo.open(48000))
		wo.start();
	RawEntryPoint();
}
