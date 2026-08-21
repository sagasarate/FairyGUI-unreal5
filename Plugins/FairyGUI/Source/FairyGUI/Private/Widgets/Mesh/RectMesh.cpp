#include "Widgets/Mesh/RectMesh.h"

void FRectMesh::BuildMesh(
	FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, TArray<FColor>* pColors)
{
	const FBox2D& rect = pSubMesh->ContentRect;
	const FColor& color = pSubMesh->VertexColor;
	int32		  StartIndex = pSubMesh->Vertices.Num();
	if (LineWidth == 0)
	{
		if (color.A != 0) // optimized
			pSubMesh->AddQuad(rect, color);
	}
	else
	{
		FBox2D	  part;
		FVector2D Position;

		// left,right
		part = FBox2D(rect.Min, rect.Min + FVector2D(LineWidth, rect.GetSize().Y));
		pSubMesh->AddQuad(part, LineColor);
		Position = FVector2D(rect.Max.X - LineWidth, rect.Min.Y);
		part = FBox2D(Position, Position + FVector2D(LineWidth, rect.GetSize().Y));
		pSubMesh->AddQuad(part, LineColor);

		// top, bottom
		Position = FVector2D(rect.Min.X + LineWidth, rect.Min.Y);
		part = FBox2D(Position, Position + FVector2D(rect.GetSize().X - LineWidth * 2, LineWidth));
		pSubMesh->AddQuad(part, LineColor);
		Position = FVector2D(rect.Min.X + LineWidth, rect.Max.Y - LineWidth);
		part = FBox2D(Position, Position + FVector2D(rect.GetSize().X - LineWidth * 2, LineWidth));
		pSubMesh->AddQuad(part, LineColor);

		// middle
		if (color.A != 0) // optimized
		{
			part = FBox2D(rect.Min + LineWidth, rect.Max - LineWidth);
			if (part.GetSize().GetMin() > 0)
				pSubMesh->AddQuad(part, color);
		}
	}
	int32 VertexCount = pSubMesh->Vertices.Num() - StartIndex;
	if (pColors)
		pSubMesh->RepeatColors(pColors->GetData(), pColors->Num(), StartIndex, VertexCount);

	pSubMesh->AddTriangles(StartIndex);
}
