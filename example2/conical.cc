#include "stdshit.h"
#include "surface.h"

// fast integer arctangent
int i16atan2Tab[65537];
void initii16atan2(void) {
	for(int i = 0; i <= 65536; i++) {
		float r = float(i-32768)/32768;
		float s = atan2((1-r)/(1+r), 1)-(M_PI/4);
		i16atan2Tab[i] = lrint(32768 * (s/M_PI));
	}
}
int __fastcall i16atan2(int y, int x)
{
    const int ONEQTR_PI = 32768 / 4;
	const int THRQTR_PI = 3 * 32768 / 4;
	const int FORQRT_PI = 4 * 32768 / 4;
	
	int angle;
	int index = 32768;
	int abs_y = y >= 0 ? y : -y;
	if ( x < 0 ) {
		int numer = 32768*(x + abs_y);
		if(int denom = abs_y - x)
			index += (numer) / (denom);
		angle = THRQTR_PI;
	} else {
		int numer = 32768*(x - abs_y);
		if(int denom = x + abs_y)
			index += (numer) / (denom);
		angle = ONEQTR_PI; }
	angle += i16atan2Tab[index];
	return (y >= 0) ? angle : 65535-angle;
}

// sinebow lookup
static inline
int sineLevel(float angle) {
	float value = sin(angle);
	return lrint(value*value*255); }
COLORREF sineBow(float angle) {
	return RGB(sineLevel(angle),
		sineLevel(angle+(M_PI*1.0/3)),
		sineLevel(angle+(M_PI*2.0/3))); }
COLORREF sineBowTab[256];
void initSineBow(void) {
	for(int i = 0; i < 256; i++) {
		float angle = M_PI*i/256;
		sineBowTab[i] = sineBow(angle);
}}

struct Conical 
{
	int angle;
	size_t xpos, ypos;
	size_t xlimit, ylimit;
	int xdelta, ydelta;
	
	void resize(int xsize, int ysize) {
		xpos = xsize/2; ypos = ysize/2;
		xlimit = xsize; ylimit = ysize;
		xdelta = 1; ydelta = 1; }

	void advcance(void) {
		angle += 1024;
		xpos += xdelta;
		if(xpos >= xlimit)
			xdelta = -xdelta;
		ypos += ydelta;
		if(ypos >= ylimit)
			ydelta = -ydelta;
	}

	RgbQuad exec(int x, int y) {
		int iatanVal = i16atan2(x-xpos, y-ypos) + angle;
		BYTE index = iatanVal >> 7;
		return *(RgbQuad*)(sineBowTab+index); }
		
	void redraw(BmSurface& bmSurface) {
		int xsize = bmSurface.bmWidth;
		int ysize = bmSurface.bmHeight;
		RgbQuad* curPos = bmSurface.getPtr(0, 0);
		for(int y = 0; y < ysize; y++)
		for(int x = 0; x < xsize; x++)
			*curPos++ = exec(x, y); 
		advcance();
			}
};

Conical conical;
BmSurface bmSurface;


// groupbox fix
void hollow_Region(HWND hwnd, RECT* outer, RECT* inner) {
	#define RECT_XY(rc) (rc).left, (rc).top
	#define RECT_LTRB(rc) RECT_XY(rc), (rc).right, (rc).bottom
	HRGN hRgn1 = CreateRectRgn(RECT_LTRB(*outer));
	HRGN hRgn2 = CreateRectRgn(RECT_LTRB(*inner));
	CombineRgn(hRgn1, hRgn1, hRgn2, RGN_DIFF);
	DeleteObject(hRgn2);
	SetWindowRgn(hwnd, hRgn1, FALSE); }
void hollow_groupBox(HWND hwnd) {
	RECT rect1; GetClientRect(hwnd, &rect1);
	HDC hdc = GetDC(hwnd); SIZE size;
	GetTextExtentPointA(hdc, "0", 1, &size);
	rect1.top += (size.cy/2)-2;
	RECT rect2 = {rect1.left+2, rect1.top+2,
		rect1.right-2, rect1.bottom-2};
	hollow_Region(hwnd, &rect1, &rect2); }
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	char className[32];
	GetClassNameA(hwnd, className, 32);
	if((!strcmp(className, "Button"))
	&&(GetWindowLongA(hwnd, GWL_STYLE) & 15) == BS_GROUPBOX)
		hollow_groupBox(hwnd);
	return TRUE;
}

extern "C"
LRESULT CALLBACK CalcWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern "C"
LRESULT CALLBACK CalcWndProc_hook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
		case WM_SIZE:
		case WM_CREATE: {
			SetWindowLongW(hwnd, GWL_STYLE, GetWindowLongW(
				hwnd, GWL_STYLE) | WS_CLIPCHILDREN);
			EnumChildWindows(hwnd, EnumChildProc, 0);
			ShowWindow(GetDlgItem(hwnd, 1000), SW_HIDE);
			initii16atan2();
			initSineBow();
			SetTimer(hwnd, 1, 10, 0);
			RECT rect; GetClientRect(hwnd, &rect);
			bmSurface.initBitmap(hwnd, rect); 
			conical.resize(rect.right, rect.bottom);
			conical.redraw(bmSurface);
			break; }
			
		case WM_TIMER: {
			HDC hdc = GetDC(hwnd);
			conical.redraw(bmSurface);
			bmSurface.drawImage(hdc);
			ReleaseDC(hwnd, hdc); }
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			bmSurface.drawImage(hdc);
			EndPaint(hwnd, &ps);
			break; }
	}
    return CalcWndProc(hwnd, msg, wParam, lParam);
}
