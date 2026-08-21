#pragma once

#include "CoreMinimal.h"

struct FObjectData
{
	FMatrix44f LocalToWorld;   // 0-63 (64 bytes, 16-byte aligned)
	FVector4f  OutlineColor;   // 64-79 (16 bytes, 16-byte aligned — 描边颜色，a=0 无描边)
	float	   Alpha;		   // 80-83
	float	   Saturation;	   // 84-87
	float	   SdfBias;		   // 88-91
	float	   SdfPixelSpread; // 92-95
	float	   OutlineBias;	   // 96-99 — 描边距离场阈值
	int		   TextDrawType;   // 100-103
	uint32	   ColorOption;	   // 104-107 — 0=straight alpha, 1=premultiplied alpha (Multiply/Screen)
	float	   _Pad;		   // 108-111 (HLSL StructuredBuffer stride 16-byte 对齐)
};