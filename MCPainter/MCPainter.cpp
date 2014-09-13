#include "stdafx.h"
#include "MCPainter.h"

#include <comdef.h>
#include <GdiPlus.h>
using namespace Gdiplus;
#include "Util.h"
#include "ColorsManager.h"
#include <stdio.h>


#pragma region
// 全局:
HINSTANCE hInst;
const TCHAR szTitle[]       = _T("MCPainter");
const TCHAR szWindowClass[] = _T("MCPainter");
HWND MainWnd, ChooseBtn, SaveBtn, ImageStatic, PreviewStatic;

// 此代码模块中包含的函数的前向声明:
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

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// 主消息循环:
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
		, (GetSystemMetrics(SM_CXSCREEN) - 654) / 2, (GetSystemMetrics(SM_CYSCREEN) - 413) / 2
		, 654, 413, NULL, NULL, hInstance, NULL);

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

// 主逻辑 ////////////////////////////////////////////////////////////////////////////////////

static ULONG_PTR gdiplusToken;                   // GDI+初始化用
static MDC BG;                                   // 图片框背景
static ComDlg dlg;                               // 通用对话框

static DWORD ImgWidth = 0, ImgHeight = 0;        // 图片宽高
static DWORD FixedWidth = 0, FixedHeight = 0;    // 缩放后图片宽高
static MDC mdc1, mdc2;                           // 内存DC，缓存图片和读取ARGB用
static int* BlockIndex = NULL;                   // 转换后各像素对应颜色表中的索引，储存顺序从左到右从下到上


// 主窗口创建
void OnCreate(HWND hWnd)
{
	// 输入颜色
	if(!InputColors())
	{
		MessageBox(MainWnd, _T("输入颜色失败！"), szTitle, MB_OK | MB_ICONERROR);
		DestroyWindow(hWnd);
		return;
	}

	// GDI+初始化
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// 图片框背景初始化
	Image Img(L"BG.png");
	BG.Create(300, 300);
	Graphics Graph(BG.GetHDC());
	Graph.DrawImage(&Img, 0, 0, Img.GetWidth(), Img.GetHeight());

	// 通用对话框初始化
	dlg.Init(hWnd, hInst);

	// 建立子窗口
	ChooseBtn     = CreateWindow(_T("Button"), _T("选择图片"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON
		, 216, 332, 100,  30, hWnd, (HMENU)ID_Choose, hInst, NULL);
	SaveBtn       = CreateWindow(_T("Button"), _T("生成"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON
		, 332, 332, 100,  30, hWnd, (HMENU)ID_Save, hInst, NULL);
	ImageStatic   = CreateWindow(_T("Static"), NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW
		,  16,  16, 300, 300, hWnd, (HMENU)ID_Image, hInst, NULL);
	PreviewStatic = CreateWindow(_T("Static"), NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW
		, 332,  16, 300, 300, hWnd, (HMENU)ID_Preview, hInst, NULL);

	// 设置字体
	HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(ChooseBtn, WM_SETFONT, (WPARAM)font, 1);
	SendMessage(SaveBtn, WM_SETFONT, (WPARAM)font, 1);
}

// 主窗口销毁
void OnDestroy()
{
	if(BlockIndex != NULL)
	{
		delete BlockIndex;
		BlockIndex = NULL;
	}
	GdiplusShutdown(gdiplusToken); 
}

// Image绘图
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

// 预览绘图
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

// 选择图片按钮按下
void Choose_OnClick()
{
	// 通用对话框选图片
	const TCHAR* Filter = _T("图片文件(*.jpg;*.png;*.bmp)\0*.jpg;*.png;*.bmp\0") \
		_T("所有文件(*.*)\0*.*\0");
	TCHAR FileName[MAX_PATH];

	if(!dlg.OpenFile(FileName, Filter, _T("jpg")))
		return;

	// 读取图片
	Image Img(FileName);
	ImgWidth  = Img.GetWidth();
	ImgHeight = Img.GetHeight();

	// 缩放
	if(ImgWidth > 300 || ImgHeight > 300)
	{
		if(ImgWidth > ImgHeight)
		{
			FixedHeight = (DWORD)((float)ImgHeight / ImgWidth * 300);
			FixedWidth  = 300;
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
		&& MessageBox(MainWnd, _T("是否缩放到300x300内？"), szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		ImgWidth  = FixedWidth;
		ImgHeight = FixedHeight;
	}

	// 画到mdc1上
	mdc1.Create(ImgWidth, ImgHeight);
	Graphics Graph(mdc1.GetHDC());
	Graph.DrawImage(&Img, 0, 0, ImgWidth, ImgHeight);

	// 转换成最接近的颜色
	mdc2.Create(ImgWidth, ImgHeight);
	int len = ImgWidth * ImgHeight, index; // 图片像素数，最近颜色索引
	if(BlockIndex != NULL)
		delete BlockIndex;
	BlockIndex = new int[len];
	for(int i = 0; i < len; i++)
	{
		if((mdc1.GetData() + i) -> byColor.A > 229) // 忽略不透明度<90%的
		{
			index = GetNearestColorIndex(*(mdc1.GetData() + i));
			BlockIndex[i] = index; //储存顺序从左到右从下到上
			(mdc2.GetData() + i) -> crColor = Colors[index].crColor;
		}
		else
			BlockIndex[i] = -1; // 忽略，空气方块
	}

	// 重画Static
	InvalidateRect(ImageStatic, NULL, TRUE);
	InvalidateRect(PreviewStatic, NULL, TRUE);
}

// 倒转int的4个字节
inline DWORD ReverseInt(DWORD x)
{
	DWORD ans;
	ans =  (x & 0xFF000000) >> 24;
	ans |= (x & 0x00FF0000) >> 8;
	ans |= (x & 0x0000FF00) << 8;
	ans |= (x & 0x000000FF) << 24;
	return ans;
}

// 倒转short的2个字节
inline DWORD ReverseShort(DWORD x)
{
	DWORD ans;
	ans =  (x & 0xFF00) >> 8;
	ans |= (x & 0x00FF) << 8;
	return ans;
}

// 生成按钮按下
void Save_OnClick()
{
    const TCHAR* Filter = _T("schematic文件(*.schematic)\0*.schematic\0");
	TCHAR FileName[MAX_PATH];

	if(BlockIndex == NULL)
	{
		MessageBox(MainWnd, _T("请先选择图片！"), szTitle, MB_OK | MB_ICONERROR);
		return;
	}

	if(!dlg.SaveFile(FileName, Filter, _T("schematic")))
		return;

//#define USE_WRITEFILE
#ifdef USE_WRITEFILE // 比fwrite慢很多！
	
	HANDLE f = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(f == INVALID_HANDLE_VALUE)
	{
		MessageBox(MainWnd, _T("打开文件失败！"), szTitle, MB_OK | MB_ICONERROR);
		return;
	}

	// 写文件
	DWORD data, tmp;
	// 头
	data = 0x0A; // TAG_Compound
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0900; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Schematic", 9, &tmp, NULL); // 标签名
	
	// 高长度 = 1
	data = 0x02; // TAG_Short
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0600; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Height", 6, &tmp, NULL); // 标签名
	data = 0x0100; // 值
	WriteFile(f, &data, 2, &tmp, NULL);

	// 纵长度
	data = 0x02; // TAG_Short
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0600; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Length", 6, &tmp, NULL); // 标签名
	data = ReverseShort(ImgHeight); // 值
	WriteFile(f, &data, 2, &tmp, NULL);

	// 横长度
	data = 0x02;//TAG_Short
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0500; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Width", 5, &tmp, NULL); // 标签名
	data = ReverseShort(ImgWidth); // 值
	WriteFile(f, &data, 2, &tmp, NULL);

	// 无实体
	data = 0x09; // TAG_List
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0800; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Entities", 8, &tmp, NULL); // 标签名
	data = 0x0A;
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0;
	WriteFile(f, &data, 4, &tmp, NULL);

	// 无方块实体
	data = 0x09; // TAG_List
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0C00; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "TileEntities", 12, &tmp, NULL); // 标签名
	data = 0x0A;
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0;
	WriteFile(f, &data, 4, &tmp, NULL);


	// 版本 = "Alpha"
	data = 0x08; // TAG_String
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0900; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Materials", 9, &tmp, NULL); // 标签名
	data = 0x0500; // 值长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Alpha", 5, &tmp, NULL); // 值

	// 方块ID
	data = 0x07; // TAG_Byte_Array
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0600; // 标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Blocks", 6, &tmp, NULL); // 标签名
	int len = ImgWidth * ImgHeight;
	data = ReverseInt(len); // 值长度
	WriteFile(f, &data, 4, &tmp, NULL);
	// *从左到右从上到下*写出方块ID
	int *pIndex, iIndex; // 当前索引指针，值
	pIndex = BlockIndex + len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1) // -1换成空气方块
				data = 0;
			else
				data = ID[iIndex];
			WriteFile(f, &data, 1, &tmp, NULL);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// 方块数据
	data = 0x07; // TAG_Byte_Array
	WriteFile(f, &data, 1, &tmp, NULL);
	data = 0x0400;//标签名长度
	WriteFile(f, &data, 2, &tmp, NULL);
	WriteFile(f, "Data", 4, &tmp, NULL); // 标签名
	data = ReverseInt(len); // 值长度
	WriteFile(f, &data, 4, &tmp, NULL);
	// *从左到右从上到下*写出方块数据
	pIndex = BlockIndex + len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1) // -1换成空气方块
				data = 0;
			else
				data = Data[iIndex];
			WriteFile(f, &data, 1, &tmp, NULL);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// 尾
	data = 0x00; // TAG_End
	WriteFile(f, &data, 1, &tmp, NULL);

	CloseHandle(f);

#else

	FILE *f;
	_tfopen_s(&f, FileName, _T("wb"));
	if(f == NULL)
	{
		MessageBox(MainWnd, _T("打开文件失败！"), szTitle, MB_OK | MB_ICONERROR);
		return;
	}

	// 写文件
	DWORD data;
	// 头
	data = 0x0A; // TAG_Compound
	fwrite(&data, 1, 1, f);
	data = 0x0900; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Schematic", 9, 1, f); // 标签名
	
	// 高长度 = 1
	data = 0x02; // TAG_Short
	fwrite(&data, 1, 1, f);
	data = 0x0600; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Height", 6, 1, f); // 标签名
	data = 0x0100; // 值
	fwrite(&data, 2, 1, f);

	// 纵长度
	data = 0x02; // TAG_Short
	fwrite(&data, 1, 1, f);
	data = 0x0600; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Length", 6, 1, f); // 标签名
	data = ReverseShort(ImgHeight); // 值
	fwrite(&data, 2, 1, f);

	// 横长度
	data = 0x02; // TAG_Short
	fwrite(&data, 1, 1, f);
	data = 0x0500; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Width", 5, 1, f); // 标签名
	data = ReverseShort(ImgWidth); // 值
	fwrite(&data, 2, 1, f);

	// 无实体
	data = 0x09; // TAG_List
	fwrite(&data, 1, 1, f);
	data = 0x0800; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Entities", 8, 1, f); // 标签名
	data = 0x0A;
	fwrite(&data, 1, 1, f);
	data = 0;
	fwrite(&data, 4, 1, f);

	// 无方块实体
	data = 0x09; // TAG_List
	fwrite(&data, 1, 1, f);
	data = 0x0C00; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("TileEntities", 12, 1, f); // 标签名
	data = 0x0A;
	fwrite(&data, 1, 1, f);
	data = 0;
	fwrite(&data, 4, 1, f);


	// 版本 = "Alpha"
	data = 0x08; // TAG_String
	fwrite(&data, 1, 1, f);
	data = 0x0900; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Materials", 9, 1, f); // 标签名
	data = 0x0500; // 值长度
	fwrite(&data, 2, 1, f);
	fwrite("Alpha", 5, 1, f); // 值

	// 方块ID
	data = 0x07; // TAG_Byte_Array
	fwrite(&data, 1, 1, f);
	data = 0x0600; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Blocks", 6, 1, f); // 标签名
	int Len = ImgWidth * ImgHeight;
	data = ReverseInt(Len); // 值长度
	fwrite(&data, 4, 1, f);
	// *从左到右从上到下*写出方块ID
	int *pIndex, iIndex; // 当前索引指针，值
	pIndex = BlockIndex + Len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1) // -1换成空气方块
				data = 0;
			else
				data = ID[iIndex];
			fwrite(&data, 1, 1, f);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// 方块数据
	data = 0x07; // TAG_Byte_Array
	fwrite(&data, 1, 1, f);
	data = 0x0400; // 标签名长度
	fwrite(&data, 2, 1, f);
	fwrite("Data", 4, 1, f); // 标签名
	data = ReverseInt(Len); // 值长度
	fwrite(&data, 4, 1, f);
	// *从左到右从上到下*写出方块数据
	pIndex = BlockIndex + Len - ImgWidth;
	for(DWORD y = 0; y < ImgHeight; y++)
	{
		for(DWORD x = 0; x < ImgWidth; x++)
		{
			iIndex = *pIndex;
			if(iIndex == -1) // -1换成空气方块
				data = 0;
			else
				data = Data[iIndex];
			fwrite(&data, 1, 1, f);
			pIndex++;
		}
		pIndex -= ImgWidth * 2;
	}

	// 尾
	data = 0x00; // TAG_End
	fwrite(&data, 1, 1, f);

	fclose(f);

#endif

	// 然后可以用gzip压缩也可以不压缩
}
