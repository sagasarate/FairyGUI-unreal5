#pragma once

#include "Widgets/Mesh/SubMesh.h"

class FAIRYGUI_API FRoundedRectMesh
{
public:
	static void BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, const FColor* pCenterColor,
		float TopLeftRadius, float TopRightRadius, float BottomLeftRadius, float BottomRightRadius);
	static bool HitTest(const FBox2D& ContentRect, const FVector2D& LocalPoint, float TopLeftRadius,
		float TopRightRadius, float BottomLeftRadius, float BottomRightRadius);
};