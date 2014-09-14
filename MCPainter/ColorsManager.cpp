#include "stdafx.h"
#include "ColorsManager.h"

#include <stdio.h>
#include <math.h>


COLOR Colors[MAXNCOLOR];                 // 颜色表
BYTE ID[MAXNCOLOR], Data[MAXNCOLOR];     // MC方块ID，数据
int nColor = 0;                          // 颜色数量


BOOL InputColors()
{
	FILE *f;
	_tfopen_s(&f, _T("Colors.txt"), _T("r"));
	if(!f)
		return FALSE;

	char str[100];
	int BlockId, BlockData, R, G, B;
	while(fgets(str, 100, f))
	{
		if(sscanf_s(str, "%d,%d,%d,%d,%d", &BlockId, &BlockData, &R, &G, &B) == 5)
		{
			ID[nColor]                 = BlockId;
			Data[nColor]               = BlockData;
			Colors[nColor].byColor.A   = 255;
			Colors[nColor].byColor.R   = R;
			Colors[nColor].byColor.G   = G;
			Colors[nColor++].byColor.B = B;
		}
	}
	fclose(f);
	return nColor > 0;
}

inline float square(float x)
{
	return x * x;
}

// 取颜色差距
float GetSquareDist(BYTE x1, BYTE y1, BYTE z1, BYTE x2, BYTE y2, BYTE z2)
{
	return square((float)(x1 - x2)) + square((float)(y1 - y2)) + square((float)(z1 - z2));
}

// 取最近颜色索引
int GetNearestColorIndex(COLOR color)
{
	// 最近颜色索引
	int index = 0;
	// 最小颜色差距
	float min = GetSquareDist(color.byColor.R, color.byColor.G, color.byColor.B,
		Colors[0].byColor.R, Colors[0].byColor.G, Colors[0].byColor.B);
	for(int i = 1; i < nColor; i++)
	{
		float dist = GetSquareDist(color.byColor.R, color.byColor.G, color.byColor.B,
			Colors[i].byColor.R, Colors[i].byColor.G, Colors[i].byColor.B);
		if(dist < min)
		{
			min = dist;
			index = i;
		}
	}
	return index;
}
