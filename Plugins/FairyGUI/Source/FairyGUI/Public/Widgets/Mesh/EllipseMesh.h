#pragma once

#include "Widgets/Mesh/SubMesh.h"

class FAIRYGUI_API FEllipseMesh
{
public:
	static void BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, const FColor* pCenterColor,
		float StartDegree, float EndDegree);
	static bool HitTest(
		const FBox2D& ContentRect, const FVector2D& LocalPoint, float LineWidth, float StartDegree, float EndDegree);
};