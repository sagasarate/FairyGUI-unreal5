#pragma once

#include "Widgets/Mesh/SubMesh.h"
#include "UI/FieldTypes.h"

class FAIRYGUI_API FFillMesh 
{
public:

	FFillMesh();
	virtual ~FFillMesh() {}

	EFillMethod Method;
	int32		Origin;
	bool		bClockwise;
	float		Amount;

	void FillMesh(FSubMesh* pSubMesh);
};