#pragma once

#include "resource.h"

const int ID_Image   = 1;
const int ID_Preview = 2;
const int ID_Choose  = 3;
const int ID_Save    = 4;

void OnCreate(HWND hWnd);
void OnDestroy();
void Image_OnDraw(HDC hdc);
void Preview_OnDraw(HDC hdc);
void Choose_OnClick();
void Save_OnClick();
