#ifndef _WAVEOUT_H_
#define _WAVEOUT_H_
#include "stdshit.h"

class WaveOut
{
public:
	WaveOut(); ~WaveOut();
	int bSize(int rate, int latency = 100);
	bool open(int rate, int latency = 100);
	void close(void); bool isPlaying(void);
	void start(void); void stop(void);
	void lock(void); void unlock(void);
	Delegate<bool, short*, int> callBack;
	
private:
	static void CALLBACK waveOutProc(
		HWAVEOUT hwo, UINT uMsg,
		DWORD_PTR dwInstance, DWORD_PTR dwParam1,
		DWORD_PTR dwParam2	);
	CRITICAL_SECTION cs;
	HWAVEOUT hWaveOut;	
	WAVEHDR* buffs;
	WORD bufferLength;
	WORD nBuffers; 
	WORD queLength;
	bool stoppingFlag;
	bool safetyFlag;
};

// IMPLEMENTATION
inline
WaveOut::WaveOut() :
	hWaveOut(0), queLength(0) { }
inline
WaveOut::~WaveOut()
	{	close(); }
inline
bool WaveOut::isPlaying(void)
	{	return queLength; }
inline	
void WaveOut::lock(void) 
	{	EnterCriticalSection(&cs);	}
inline
void WaveOut::unlock(void) 
	{	LeaveCriticalSection(&cs);	}

#endif
