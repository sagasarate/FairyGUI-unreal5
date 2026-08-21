#pragma once

#include "Widgets/Mesh/SubMesh.h"

class FAIRYGUI_API FRegularPolygonMesh
{
public:
	static void BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, int32 Sides,
		const TArray<float>& Distances, float Rotation, const FColor* pCenterColor);
	static bool HitTest(const FBox2D& ContentRect, const FVector2D& LocalPoint, int32 Sides,
		const TArray<float>& Distances, float Rotation);
};