#include "stdafx.h"
#include "Util.h"


// ComDlg ////////////////////////////////////////////////////////////////////

void ComDlg::Init(HWND hwnd, HINSTANCE hInstance)
{
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = hwnd;
	ofn.hInstance   = hInstance;
	ofn.nMaxFile    = MAX_PATH;
}

BOOL ComDlg::OpenFile(LPTSTR pstrFile, LPCTSTR szFilter, LPCTSTR szDefExt)
{
	ofn.lpstrFile   = pstrFile;
	pstrFile[0]     = '\0';
	ofn.lpstrFilter = szFilter;
	ofn.lpstrDefExt = szDefExt;
	ofn.Flags       = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	return GetOpenFileName(&ofn);
}

BOOL ComDlg::SaveFile(LPTSTR pstrFile, LPCTSTR szFilter, LPCTSTR szDefExt)
{
	ofn.lpstrFile   = pstrFile;
	pstrFile[0]     = '\0';
	ofn.lpstrFilter = szFilter;
	ofn.lpstrDefExt = szDefExt;
	ofn.Flags       = OFN_OVERWRITEPROMPT;

	return GetSaveFileName(&ofn);
}

// MDC ////////////////////////////////////////////////////////////////////

MDC::MDC()
{
	ZeroMemory(&info, sizeof(info));
	info.bmiHeader.biSize     = sizeof(info.bmiHeader);
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biPlanes   = 1;
}

MDC::~MDC()
{
	Destroy();
}

void MDC::Create(int width, int height)
{
	Destroy();
	hdc = CreateCompatibleDC(NULL);

	info.bmiHeader.biWidth     = width;
	info.bmiHeader.biHeight    = height;
	info.bmiHeader.biSizeImage = 4 * width * height;
	Bitmap = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, (void**)&data, NULL, 0);

	OldBitmap = (HBITMAP)SelectObject(hdc, Bitmap);
}

void MDC::Destroy()
{
	if(hdc != NULL)
	{
		SelectObject(hdc, OldBitmap);
		DeleteObject(Bitmap);
		DeleteDC(hdc);
		Bitmap = NULL;
		hdc = NULL;
	}
}
