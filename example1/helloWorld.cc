#include <stdio.h>
#include <process.h>
#include <windows.h>

extern "C" __stdcall
int  SetDisplayText(HWND hDlg, LPCWSTR lpString);
extern "C" __stdcall
int  SetDisplayText_hook(HWND hDlg, LPCWSTR lpString)
{
	WCHAR tmp[256];
	wsprintfW(tmp, L"DeadFish Shitware, %s", lpString);
	SetDisplayText(hDlg, tmp);
}

extern "C"
void RawEntryPoint(void);
extern "C"
void HookEntryPoint(void)
{
	MessageBoxW(NULL, L"hello world", L"hello world", MB_OK);
	RawEntryPoint();
}
