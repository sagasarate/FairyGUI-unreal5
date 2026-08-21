#include "Widgets/Mesh/RegularPolygonMesh.h"

void FRegularPolygonMesh::BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, int32 Sides,
	const TArray<float>& Distances, float Rotation, const FColor* pCenterColor)
{
	if (Distances.Num() > 0)
		verifyf(Distances.Num() >= Sides, TEXT("Distances.Length<Sides"));

	const FBox2D& rect = pSubMesh->ContentRect;
	const FColor& color = pSubMesh->VertexColor;
	int32		  StartIndex = pSubMesh->Vertices.Num();

	float angleDelta = 2 * PI / Sides;
	float angle = FMath::DegreesToRadians(Rotation);
	float radius = rect.GetSize().GetMin() * 0.5f;

	FVector2D center = FVector2D(radius, radius) + rect.Min;
	pSubMesh->AddVertex(center, pCenterColor ? *pCenterColor : color);
	for (int32 i = 0; i < Sides; i++)
	{
		float r = radius;
		if (Distances.Num() > 0)
			r *= Distances[i];
		FVector2D vec = center + FVector2D(FMath::Cos(angle) * (r - LineWidth), FMath::Sin(angle) * (r - LineWidth));
		pSubMesh->AddVertex(vec, color);
		if (LineWidth > 0)
		{
			pSubMesh->AddVertex(vec, LineColor);

			vec = FVector2D(FMath::Cos(angle) * r + center.X, FMath::Sin(angle) * r + center.Y);
			pSubMesh->AddVertex(vec, LineColor);
		}
		angle += angleDelta;
	}

	if (LineWidth > 0)
	{
		int32 tmp = Sides * 3;
		for (int32 i = 0; i < tmp; i += 3)
		{
			if (i != tmp - 3)
			{
				pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + i + 4);
				pSubMesh->AddTriangle(StartIndex + i + 5, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + i + 6, StartIndex + i + 5);
			}
			else
			{
				pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + 1);
				pSubMesh->AddTriangle(StartIndex + 2, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + 3, StartIndex + 2);
			}
		}
	}
	else
	{
		for (int32 i = 0; i < Sides; i++)
			pSubMesh->AddTriangle(
				StartIndex, StartIndex + i + 1, (i == Sides - 1) ? StartIndex + 1 : StartIndex + i + 2);
	}
}

bool FRegularPolygonMesh::HitTest(
	const FBox2D& ContentRect, const FVector2D& LocalPoint, int32 Sides, const TArray<float>& Distances, float Rotation)
{
	float	  angleDelta = 2 * PI / Sides;
	float	  angle = FMath::DegreesToRadians(Rotation);
	float	  radius = ContentRect.GetSize().GetMin() * 0.5f;
	FVector2D center = FVector2D(radius, radius) + ContentRect.Min;

	TArray<FVector2D> Points;
	Points.SetNum(Sides);
	for (int32 i = 0; i < Sides; i++)
	{
		float r = radius;
		if (Distances.Num() > 0)
			r *= Distances[i];
		Points[i] = center + FVector2D(FMath::Cos(angle) * r, FMath::Sin(angle) * r);
		angle += angleDelta;
	}

	int32 len = Points.Num();
	int32 j = len - 1;
	bool  oddNodes = false;

	for (int32 i = 0; i < len; ++i)
	{
		const FVector2D& vi = Points[i];
		const FVector2D& vj = Points[j];

		if (((vi.Y < LocalPoint.Y && vj.Y >= LocalPoint.Y) || (vj.Y < LocalPoint.Y && vi.Y >= LocalPoint.Y))
			&& (vi.X <= LocalPoint.X || vj.X <= LocalPoint.X))
		{
			if (vi.X + (LocalPoint.Y - vi.Y) / (vj.Y - vi.Y) * (vj.X - vi.X) < LocalPoint.X)
				oddNodes = !oddNodes;
		}

		j = i;
	}

	return oddNodes;
}
