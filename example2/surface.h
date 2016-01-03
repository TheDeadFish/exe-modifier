#ifndef _SURFACE_H_
#define _SURFACE_H_

struct RgbQuad {
	RgbQuad() {}
	RgbQuad(DWORD argb) {
		this->argb = argb; }
	RgbQuad(BYTE r, BYTE g, BYTE b) {
		rgbRed = r; rgbBlue = r; rgbGreen = r; }
	union {	DWORD argb;
	struct { BYTE rgbBlue; BYTE rgbGreen;
		BYTE rgbRed; BYTE rgbAlpha;	};};
};

struct BmSurface
{
	HDC bmHdc;
	RgbQuad* bmBits;
	DWORD bmWidth;
	DWORD bmHeight;
	HWND hwnd; RECT rc;
	
	bool initBitmap(HWND hwnd, RECT& rc);
	void drawImage(HDC hdc);
	RgbQuad* getPtr(DWORD x, DWORD y) {
		return &bmBits[x + y*bmWidth]; }
	RgbQuad& getRef(DWORD x, DWORD y) {
		return bmBits[x + y*bmWidth]; }
};

#endif
