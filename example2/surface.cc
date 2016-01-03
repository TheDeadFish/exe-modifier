#include "stdshit.h"
#include "surface.h"

bool BmSurface::initBitmap(HWND hwnd, RECT& rc)
{
	// calculate width/height
	int width = max(rc.right-rc.left, 1);
	int height = max(rc.bottom-rc.top, 1);
	this->hwnd = hwnd;
	*(POINT*)&this->rc = *(POINT*)&rc;
	this->rc.right = rc.left + width;
	this->rc.bottom = rc.top + height;
	
	// check for change
	if(( bmBits != NULL ) 
	&&( this->bmWidth == width )
	&&( this->bmHeight == height ))
		return false;
	this->bmWidth = width;
	this->bmHeight = height;
	
	// Delete old bitmap
	if( bmHdc == NULL )
		bmHdc = CreateCompatibleDC(NULL);
	DeleteObject(SelectObject(bmHdc,
		GetStockObject(21)));
	
	// Create bitmap
	BITMAPINFO bmInfo = {0};
	bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
	bmInfo.bmiHeader.biWidth = this->bmWidth;
	bmInfo.bmiHeader.biHeight = -this->bmHeight;
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 32;
	if(! SelectObject(bmHdc, CreateDIBSection(bmHdc, &bmInfo,
	  DIB_RGB_COLORS, (VOID**)&bmBits, NULL, 0)))
		errorAlloc();
	return true;
}

void BmSurface::drawImage(HDC hdc)
{
	BitBlt(hdc, rc.left, rc.top, rc.right-rc.left,
		rc.bottom-rc.top, bmHdc, 0, 0, SRCCOPY);
}
