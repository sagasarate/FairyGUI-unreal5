#pragma once

#include "Widgets/Mesh/SubMesh.h"

class FAIRYGUI_API FPolygonMesh
{
public:
	static void BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, const TArray<FVector2D>& Points,
		bool bUsePercentPositions, TArray<FColor>* pColors, TArray<FVector2D>* pTexcoords);
	static bool HitTest(const FBox2D& ContentRect, const FVector2D& LocalPoint, const TArray<FVector2D>& Points,
		bool bUsePercentPositions);

private:
	static void DrawOutline(
		FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, const TArray<FVector2D>& Points);
	static bool IsPointInTriangle(const FVector2D& p, const FVector2D& a, const FVector2D& b, const FVector2D& c);
};