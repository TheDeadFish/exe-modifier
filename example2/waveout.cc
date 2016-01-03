#include "waveout.h"

int WaveOut::bSize(int rate, int latency)
{
	int buffTime = ((latency + 200) / 200);
	if(( buffTime == 1)
	&&( rate != 44100 )&&( rate != 48000 )
	&&( rate != 96000 )&&( rate != 19200 ))
		buffTime = 2;
	return rate * buffTime / 100;
}

bool WaveOut::open(int rate, int latency)
{
	assert(hWaveOut == NULL);

	// calculate buffers
	bufferLength = bSize(rate, latency);
	int tmpVar = bufferLength * 1000;
	nBuffers = ((latency * rate) + (tmpVar-1)) / tmpVar;
	if(nBuffers < 4) nBuffers = 4;
	
	// open waveout device
	MMRESULT result; WAVEFORMATEX wfx;
	wfx.nSamplesPerSec = rate;
	wfx.wBitsPerSample = 16;
	wfx.nChannels = 2;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx,
		(DWORD_PTR)&waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if(result != MMSYSERR_NOERROR)
		return false;
	InitializeCriticalSection(&cs);

	// setup buffers
	buffs = xmalloc( nBuffers*sizeof(WAVEHDR) +
		nBuffers*bufferLength*4 );
	ZeroMemory(buffs, nBuffers*sizeof(WAVEHDR));
	buffs[0].lpData = (char*)&buffs[nBuffers];
	for(int i = 0; i < nBuffers; i++)
	{
		buffs[i].lpData = buffs[0].lpData + i*bufferLength*4;
		buffs[i].dwBufferLength = bufferLength*4;
		result = waveOutPrepareHeader(hWaveOut, &buffs[i], sizeof(WAVEHDR));
		if( result != MMSYSERR_NOERROR )
			return (this->close(), false);
	}
	return true;
}

void WaveOut::close()
{
	if(hWaveOut == NULL)
		return;
	this->stop();
	for(int i = 0; i < nBuffers; i++)
	waveOutUnprepareHeader(hWaveOut,
		&buffs[i], sizeof(WAVEHDR));
	free(buffs);
	waveOutClose(hWaveOut);
	hWaveOut = NULL;
	DeleteCriticalSection(&cs);
}

void WaveOut::start(void)
{
	this->stop();
	waveOutPause(hWaveOut);
	stoppingFlag = false;
	queLength = 0;
	for(int i = 0; i < nBuffers; i++)
	{
		if(!callBack((short*)buffs[i].lpData, bufferLength))
		{
			stoppingFlag = true;
			break;
		}
		waveOutWrite(hWaveOut, &buffs[i], sizeof(WAVEHDR));
		queLength += 1;
	}
	waveOutRestart(hWaveOut);
}

void WaveOut::stop(void)
{	
	this->lock();
	safetyFlag = true;
	this->unlock();
	waveOutReset(hWaveOut);
	safetyFlag = false;
	//threadBoost = false;
	queLength = 0;
}

void CALLBACK WaveOut::waveOutProc(
	HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,	DWORD_PTR dwParam2	
){
	if(uMsg != WOM_DONE) return;
	WaveOut* This = (WaveOut*)dwInstance;
	This->lock();
	if(This->safetyFlag == true)
		goto SAFETY_LEAVE;
	if( This->stoppingFlag == true )
	{
STOP_PLAYBACK:
		This->stoppingFlag = true;
		if( --This->queLength == 0)
			This->callBack(NULL, 0);
	}
	else
	{
		WAVEHDR* lpwvhdr = (WAVEHDR*)dwParam1;
		bool result = This->callBack((short*)
			lpwvhdr->lpData, This->bufferLength);
		if( result == false )	
			goto STOP_PLAYBACK;
		waveOutWrite(This->hWaveOut, lpwvhdr, sizeof(WAVEHDR));
	}
SAFETY_LEAVE:
	This->unlock();
}
