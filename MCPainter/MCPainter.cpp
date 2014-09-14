#include "stdafx.h"
#include "MCPainter.h"

#include <comdef.h>
#include <GdiPlus.h>
using namespace Gdiplus;
#include "Util.h"
#include "ColorsManager.h"
#include <stdio.h>


#pragma region
// global:
HINSTANCE hInst;
const TCHAR szTitle[]       = _T("MCPainter");
const TCHAR szWindowClass[] = _T("MCPainter");
HWND MainWnd, ChooseBtn, SaveBtn, ImageStatic, PreviewStatic;

// declarations:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR	lpCmdLine,
					 int	   nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;

	MyRegisterClass(hInstance);

	// initialize:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MCPAINTER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MCPAINTER));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	MainWnd = CreateWindow(szWindowClass, szTitle, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_GROUP
		, (GetSystemMetrics(SM_CXSCREEN) - 654) / 2, (GetSystemMetrics(SM_CYSCREEN) - 405) / 2
		, 654, 405, NULL, NULL, hInstance, NULL);

	if (!MainWnd)
	{
		return FALSE;
	}

	ShowWindow(MainWnd, nCmdShow);
	UpdateWindow(MainWnd);
	
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis;

	switch (message)
	{
	case WM_CREATE:
		OnCreate(hWnd);
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
			switch (LOWORD(wParam))
			{
			case ID_Choose:
				Choose_OnClick();
				break;
			case ID_Save:
				Save_OnClick();
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		break;
	case WM_DRAWITEM:
		pdis = (LPDRAWITEMSTRUCT)lParam;
		switch(pdis->CtlID)
		{
			case ID_Image:
				Image_OnDraw(pdis->hDC);
				break;
			case ID_Preview:
				Preview_OnDraw(pdis->hDC);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		OnDestroy();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#pragma endregion

// main logic ////////////////////////////////////////////////////////////////////////////////////

static ULONG_PTR gdiplusToken;                   // used to initialize GDI+
static MDC BG;                                   // the background of static control
static ComDlg dlg;                               // common dialog

static DWORD ImgWidth = 0, ImgHeight = 0;
static DWORD FixedWidth = 0, FixedHeight = 0;    // the width and height after resizing
static MDC mdc1, mdc2;                           // memory DC used to store image and read ARGB
static int* BlockIndex = NULL;                   // the index of colors of each pixel, from left to right and from top to bottom


void OnCreate(HWND hWnd)
{
	if(!InputColors())
	{
		MessageBox(MainWnd, _T("Failed to input colors!"), szTitle, MB_OK | MB_ICONERROR);
		DestroyWindow(hWnd);
		return;
	}

	// initialize GDI+
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// load the background of static control
	Image Img(L"BG.png");
	BG.Create(300, 300);
	Graphics Graph(BG.GetHDC());
	Graph.DrawImage(&Img, 0, 0, Img.GetWidth(), Img.GetHeight());

	// initialize the common dialog
	dlg.Init(hWnd, hInst);

	// create controls
	ChooseBtn     = CreateWindow(_T("Button"), _T("Browse"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON
		, 216, 332, 100,  30, hWnd, (HMENU)ID_Choose, hInst, NULL);
	SaveBtn       = CreateWindow(_T("Button"), _T("Save"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON
		, 332, 332, 100,  30, hWnd, (HMENU)ID_Save, hInst, NULL);
	ImageStatic   = CreateWindow(_T("Static"), NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW
		,  16,  16, 300, 300, hWnd, (HMENU)ID_Image, hInst, NULL);
	PreviewStatic = CreateWindow(_T("Static"), NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW
		, 332,  16, 300, 300, hWnd, (HMENU)ID_Preview, hInst, NULL);

	// set the font of buttons
	HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(ChooseBtn, WM_SETFONT, (WPARAM)font, 1);
	SendMessage(SaveBtn, WM_SETFONT, (WPARAM)font, 1);
}

void OnDestroy()
{
	if(BlockIndex != NULL)
	{
		delete BlockIndex;
		BlockIndex = NULL;
	}
	GdiplusShutdown(gdiplusToken); 
}

void Image_OnDraw(HDC hdc)
{
	RECT rect = {0, 0, 300, 300};
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	BLENDFUNCTION bf;
	bf.BlendOp             = AC_SRC_OVER;
	bf.BlendFlags          = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat         = 1; // AC_SRC_NO_PREMULT_ALPHA

	AlphaBlend(hdc, 0, 0, 300, 300, BG.GetHDC(), 0, 0, 300, 300, bf);
	if(mdc1.GetHDC() != NULL)
		AlphaBlend(hdc, 0, 0, FixedWidth, FixedHeight, mdc1.GetHDC(), 0, 0, ImgWidth, ImgHeight, bf);
	FrameRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

void Preview_OnDraw(HDC hdc)
{
	RECT rect = {0, 0, 300, 300};
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	BLENDFUNCTION bf;
	bf.BlendOp             = AC_SRC_OVER;
	bf.BlendFlags          = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat         = 1; // AC_SRC_NO_PREMULT_ALPHA

	AlphaBlend(hdc, 0, 0, 300, 300, BG.GetHDC(), 0, 0, 300, 300, bf);
	if(mdc2.GetHDC() != NULL)
		AlphaBlend(hdc, 0, 0, FixedWidth, FixedHeight, mdc2.GetHDC(), 0, 0, ImgWidth, ImgHeight, bf);
	FrameRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

void Choose_OnClick()
{
	const TCHAR* Filter = _T("Image files(*.jpg;*.png;*.bmp)\0*.jpg;*.png;*.bmp\0") \
		_T("All files(*.*)\0*.*\0");
	TCHAR FileName[MAX_PATH];

	if(!dlg.OpenFile(FileName, Filter, _T("jpg")))
		return;

	// load the image
	Image Img(FileName);
	ImgWidth  = Img.GetWidth();
	ImgHeight = Img.GetHeight();

	// resize
	if(ImgWidth > 300 || ImgHeight > 300)
	{
		if(ImgWidth > ImgHeight)
		{
			FixedWidth  = 300;
			FixedHeight = (DWORD)((float)ImgHeight / ImgWidth * 300);
		}
		else
		{
			FixedWidth  = (DWORD)((float)ImgWidth / ImgHeight * 300);
			FixedHeight = 300;
		}
	}
	else
	{
		FixedWidth  = ImgWidth;
		FixedHeight = ImgHeight;
	}
	if((ImgWidth > 300 || ImgHeight > 300)
		&& MessageBox(MainWnd, _T("Your image is too large, Do you want to narrow it?"), szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		ImgWidth  = FixedWidth;
		ImgHeight = FixedHeight;
	}

	// draw it to mdc1
	mdc1.Create(ImgWidth, ImgHeight);
	Graphics Graph(mdc1.GetHDC());
	Graph.DrawImage(&Img, 0, 0, ImgWidth, ImgHeight);

	// convert its colors with colors table
	mdc2.Create(ImgWidth, ImgHeight);
	int len = ImgWidth * ImgHeight, index; // length of the image, index of the closest color
	if(BlockIndex != NULL)
		delete BlockIndex;
	BlockIndex = new int[len];
	for(int i = 0; i < len; i++)
	{
		if((mdc1.GetData() + i) -> byColor.A > 229) // replace pixels whose Alpha < 90% with air block
		{
			index = GetNearestColorIndex(*(mdc1.GetData() + i));
			BlockIndex[i] = index; // from left to right and from top to bottom
			(mdc2.GetData() + i) -> crColor = Colors[index].crColor;
		}
		else
			BlockIndex[i] = -1; // -1 stands for air block
	}

	// redraw statics
	InvalidateRect(ImageStatic, NULL, TRUE);
	InvalidateRect(PreviewStatic, NULL, TRUE);
}

// reverse 4 bytes
inline DWORD ReverseInt(DWORD x)
{
	DWORD ans;
	ans =  (x & 0xFF000000) >> 24;
	ans |= (x & 0x00FF0000) >> 8;
	ans |= (x & 0x0000FF00) << 8;
	ans |= (x & 0x000000FF) << 24;
	return ans;
}

// reverse 2 bytes
inline DWORD ReverseShort(DWORD x)
{
	DWORD ans;
	ans =  (x & 0xFF00) >> 8;
	ans |= (x & 0x00FF) << 8;
	return ans;
}

void Save_OnClick()
{
    const TCHAR* Filter = _T("schematic files(*.schematic)\0*.schematic\0");
	TCHAR FileName[MAX_PATH];

	if(BlockIndex == NULL)
	{
		MessageBox(MainWnd, _T("Please choose an image first£¡"), szTitle, MB_OK | MB_ICONERROR);
		return;
	}

	if(!dlg.SaveFile(FileName, Filter, _T("schematic")))
		return;

//#define USE_WRITEFILE
#ifdef USE_WRITEFILE // somehow very slow!
	
	HANDLE f = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(f == INVALID_HANDLE_VALUE)
	{
		MessageBox(MainWnd, _T("Failed to create file!"), szTitle, MB_OK | MB_ICONERROR);
		return;
	}

	DWORD data, tmp;
	// head
	data = 0x0A;                              // TAG_Compound
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0900;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Schematic", 9, &tmp, NULL); // tag name
	
	// length in Y = 1
	data = 0x02;                              // TAG_Short
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0600;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Height", 6, &tmp, NULL);    // tag name
	data = 0x0100;                            // value
	WriteFile(f, &data, 2, &tmp, NULL);

	// length in Z
	data = 0x02;                              // TAG_Short
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0600;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Length", 6, &tmp, NULL);    // tag name
	data = ReverseShort(ImgHeight);           // value
	WriteFile(f, &data, 2, &tmp, NULL);

	// length in X
	data = 0x02;                              // TAG_Short
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0500;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Width", 5, &tmp, NULL);     // tag name
	data = ReverseShort(ImgWidth);            // value
	WriteFile(f, &data, 2, &tmp, NULL);

	// no entity
	data = 0x09;                              // TAG_List
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0800;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Entities", 8, &tmp, NULL);  // tag name
	data = 0x0A;
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0;
	WriteFile(f, &data, 4, &tmp, NULL);

	// no tile entity
	data = 0x09;                                  // TAG_List
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0C00;                                // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "TileEntities", 12, &tmp, NULL); // tag name
	data = 0x0A;
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0;
	WriteFile(f, &data, 4, &tmp, NULL);


	// version = "Alpha"
	data = 0x08;                              // TAG_String
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0900;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Materials", 9, &tmp, NULL); // tag name
	data = 0x0500;                            // the lenth of value
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Alpha", 5, &tmp, NULL);     // value

	// block ID
	data = 0x07;                              // TAG_Byte_Array
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0600;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Blocks", 6, &tmp, NULL);    // tag name
	int len = ImgWidth * ImgHeight;
	data = ReverseInt(len);                   // the lenth of value
	WriteFile(f, &data, 4, &tmp, NULL);
	// write block ID *from left to right and from top to bottom*
	int *pIndex, iIndex;                      // pointer to current block index, block index
	pIndex = BlockIndex + len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1)                  // replace -1 with air block
				data = 0;
			else
				data = ID[iIndex];
			WriteFile(f, &data, 1, &tmp, NULL);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// blcok data
	data = 0x07;                              // TAG_Byte_Array
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0400;                            // the lenth of tag name
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Data", 4, &tmp, NULL);      // tag name
	data = ReverseInt(len);                   // the lenth of value
	WriteFile(f, &data, 4, &tmp, NULL);
	// write block data *from left to right and from top to bottom*
	pIndex = BlockIndex + len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1)                  // replace -1 with air block
				data = 0;
			else
				data = Data[iIndex];
			WriteFile(f, &data, 1, &tmp, NULL);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// end
	data = 0x00;                             // TAG_End
	WriteFile(f, &data, 1, &tmp, NULL);

	CloseHandle(f);

#else

	FILE *f;
	_tfopen_s(&f, FileName, _T("wb"));
	if(f == NULL)
	{
		MessageBox(MainWnd, _T("Failed to create file!"), szTitle, MB_OK | MB_ICONERROR);
		return;
	}

	DWORD data;
	// head
	data = 0x0A;                              // TAG_Compound
	fwrite(&data, 1, 1, f);
	data = 0x0900;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Schematic", 9, 1, f);             // tag name
	
	// length in Y = 1
	data = 0x02;                              // TAG_Short
	fwrite(&data, 1, 1, f);
	data = 0x0600;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Height", 6, 1, f);                // tag name
	data = 0x0100;                            // value
	fwrite(&data, 2, 1, f);

	// length in Z
	data = 0x02;                              // TAG_Short
	fwrite(&data, 1, 1, f);
	data = 0x0600;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Length", 6, 1, f);                // tag name
	data = ReverseShort(ImgHeight);           // value
	fwrite(&data, 2, 1, f);

	// length in X
	data = 0x02;                              // TAG_Short
	fwrite(&data, 1, 1, f);
	data = 0x0500;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Width", 5, 1, f);                 // tag name
	data = ReverseShort(ImgWidth);            // value
	fwrite(&data, 2, 1, f);

	// no entity
	data = 0x09;                              // TAG_List
	fwrite(&data, 1, 1, f);
	data = 0x0800;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Entities", 8, 1, f);              // tag name
	data = 0x0A;
	fwrite(&data, 1, 1, f);
	data = 0;
	fwrite(&data, 4, 1, f);

	// no tile entity
	data = 0x09;                              // TAG_List
	fwrite(&data, 1, 1, f);
	data = 0x0C00;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("TileEntities", 12, 1, f);         // tag name
	data = 0x0A;
	fwrite(&data, 1, 1, f);
	data = 0;
	fwrite(&data, 4, 1, f);


	// version = "Alpha"
	data = 0x08;                              // TAG_String
	fwrite(&data, 1, 1, f);
	data = 0x0900;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Materials", 9, 1, f);             // tag name
	data = 0x0500;                            // the lenth of value
	fwrite(&data, 2, 1, f);
	fwrite("Alpha", 5, 1, f);                 // value

	// block ID
	data = 0x07;                              // TAG_Byte_Array
	fwrite(&data, 1, 1, f);
	data = 0x0600;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Blocks", 6, 1, f);                // tag name
	int Len = ImgWidth * ImgHeight;
	data = ReverseInt(Len);                   // the lenth of value
	fwrite(&data, 4, 1, f);
	// write block ID *from left to right and from top to bottom*
	int *pIndex, iIndex;                      // pointer to current block index, block index
	pIndex = BlockIndex + Len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1)                  // replace -1 with air block
				data = 0;
			else
				data = ID[iIndex];
			fwrite(&data, 1, 1, f);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// block data
	data = 0x07;                              // TAG_Byte_Array
	fwrite(&data, 1, 1, f);
	data = 0x0400;                            // the lenth of tag name
	fwrite(&data, 2, 1, f);
	fwrite("Data", 4, 1, f);                  // tag name
	data = ReverseInt(Len);                   // the lenth of value
	fwrite(&data, 4, 1, f);
	// write block data *from left to right and from top to bottom*
	pIndex = BlockIndex + Len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1)                  // replace -1 with air block
				data = 0;
			else
				data = Data[iIndex];
			fwrite(&data, 1, 1, f);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// end
	data = 0x00;                             // TAG_End
	fwrite(&data, 1, 1, f);

	fclose(f);

#endif

	// then you can gzip it or not
}
