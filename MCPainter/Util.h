#pragma once

#include <Commdlg.h>

class ComDlg
{
public:
	void Init(HWND hwnd, HINSTANCE hInstance);
	BOOL OpenFile(LPTSTR pstrFile, LPCTSTR szFilter, LPCTSTR szDefExt);
	BOOL SaveFile(LPTSTR pstrFile, LPCTSTR szFilter, LPCTSTR szDefExt);

private:
	OPENFILENAME ofn;
};

union COLOR
{
	COLORREF crColor;
	struct
	{
		BYTE B, G, R, A;
	} byColor;
};

class MDC
{
public:
	MDC();
	~MDC();

	void Create(int width, int height);
	void Destroy();
	HDC GetHDC()
	{
		return hdc;
	}
	COLOR* GetData()
	{
		return data;
	}

private:
	BITMAPINFO info;
	HBITMAP OldBitmap;
	HBITMAP Bitmap;
	HDC hdc;
	COLOR* data;
};
