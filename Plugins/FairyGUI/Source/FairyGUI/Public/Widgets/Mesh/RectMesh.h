#pragma once

#include "Widgets/Mesh/SubMesh.h"

class FAIRYGUI_API FRectMesh
{
public:
	static void BuildMesh(
		FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, TArray<FColor>* pColors = nullptr);
};