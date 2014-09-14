#pragma once

#include "Util.h"

const int MAXNCOLOR = 1000;
extern int nColor;                              // 颜色数量
extern COLOR Colors[MAXNCOLOR];                 // 颜色表
extern BYTE ID[MAXNCOLOR], Data[MAXNCOLOR];     // MC方块ID，数据

BOOL InputColors();
float GetDist(BYTE R1, BYTE G1, BYTE B1, BYTE R2, BYTE G2, BYTE B2);
int GetNearestColorIndex(COLOR ParamColor);
