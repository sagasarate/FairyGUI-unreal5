#include "Widgets/Mesh/EllipseMesh.h"

void FEllipseMesh::BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor, const FColor* pCenterColor,
	float StartDegree, float EndDegree)
{
	static SlateIndex SECTOR_CENTER_TRIANGLES[] = { 0, 4, 1, 0, 3, 4, 0, 2, 3, 0, 8, 5, 0, 7, 8, 0, 6, 7, 6, 5, 2, 2, 1,
		6 };

	const FBox2D& rect = pSubMesh->ContentRect;
	const FColor& color = pSubMesh->VertexColor;
	int32		  StartIndex = pSubMesh->Vertices.Num();

	float sectionStart = FMath::Clamp<float>(StartDegree, 0, 360);
	float sectionEnd = FMath::Clamp<float>(EndDegree, 0, 360);
	bool  clipped = sectionStart > 0 || sectionEnd < 360;
	sectionStart = FMath::DegreesToRadians(sectionStart);
	sectionEnd = FMath::DegreesToRadians(sectionEnd);
	const FColor& centerColor2 = pCenterColor ? *pCenterColor : color;

	bool bDrawCenter = color.A != 0;

	FVector2D radius = rect.GetSize() * 0.5f;
	int32	  sides = FMath::CeilToInt(PI * (radius.X + radius.Y) / 4);
	sides = FMath::Clamp<int32>(sides, 40, 800);
	float angleDelta = 2 * PI / sides;
	float angle = 0;
	float lineAngle = 0;

	if (LineWidth > 0 && clipped)
	{
		lineAngle = LineWidth / radius.GetMax();
		sectionStart += lineAngle;
		sectionEnd -= lineAngle;
	}

	FVector2D center = rect.Min + radius;
	pSubMesh->AddVertex(center, centerColor2);
	for (int32 i = 0; i < sides; i++)
	{
		if (angle < sectionStart)
			angle = sectionStart;
		else if (angle > sectionEnd)
			angle = sectionEnd;
		FVector2D vec(FMath::Cos(angle) * (radius.X - LineWidth) + center.X,
			FMath::Sin(angle) * (radius.Y - LineWidth) + center.Y);
		pSubMesh->AddVertex(vec, color);
		if (LineWidth > 0)
		{
			pSubMesh->AddVertex(vec, LineColor);
			pSubMesh->AddVertex(
				FVector2D(FMath::Cos(angle) * radius.X + center.X, FMath::Sin(angle) * radius.Y + center.Y), LineColor);
		}
		angle += angleDelta;
	}

	if (LineWidth > 0)
	{
		int32 cnt = sides * 3;
		for (int32 i = 0; i < cnt; i += 3)
		{
			if (i != cnt - 3)
			{
				if (bDrawCenter)
					pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + i + 4);
				pSubMesh->AddTriangle(StartIndex + i + 5, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + i + 6, StartIndex + i + 5);
			}
			else if (!clipped)
			{
				if (bDrawCenter)
					pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + 1);
				pSubMesh->AddTriangle(StartIndex + 2, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + 3, StartIndex + 2);
			}
			else
			{
				if (bDrawCenter)
					pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + i + 1);
				pSubMesh->AddTriangle(StartIndex + i + 2, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + i + 3, StartIndex + i + 2);
			}
		}
	}
	else if (bDrawCenter)
	{
		for (int32 i = 0; i < sides; i++)
		{
			if (i != sides - 1)
				pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + i + 2);
			else if (!clipped)
				pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + 1);
			else
				pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + i + 1);
		}
	}

	if (LineWidth > 0 && clipped)
	{
		pSubMesh->AddVertex(FVector2D(radius.X, radius.Y), LineColor);
		float centerRadius = LineWidth * 0.5f;

		sectionStart -= lineAngle;
		angle = sectionStart + lineAngle * 0.5f + PI * 0.5f;
		pSubMesh->AddVertex(
			FVector2D(FMath::Cos(angle) * centerRadius + radius.X, FMath::Sin(angle) * centerRadius + radius.Y),
			LineColor);
		angle -= PI;
		pSubMesh->AddVertex(
			FVector2D(FMath::Cos(angle) * centerRadius + radius.X, FMath::Sin(angle) * centerRadius + radius.Y),
			LineColor);
		pSubMesh->AddVertex(
			FVector2D(FMath::Cos(sectionStart) * radius.X + radius.X, FMath::Sin(sectionStart) * radius.Y + radius.Y),
			LineColor);
		pSubMesh->AddVertex(pSubMesh->GetPosition(StartIndex + 3), LineColor);

		sectionEnd += lineAngle;
		angle = sectionEnd - lineAngle * 0.5f + PI * 0.5f;
		pSubMesh->AddVertex(
			FVector2D(FMath::Cos(angle) * centerRadius + radius.X, FMath::Sin(angle) * centerRadius + radius.Y),
			LineColor);
		angle -= PI;
		pSubMesh->AddVertex(
			FVector2D(FMath::Cos(angle) * centerRadius + radius.X, FMath::Sin(angle) * centerRadius + radius.Y),
			LineColor);
		pSubMesh->AddVertex(pSubMesh->GetPosition(StartIndex + sides * 3), LineColor);
		pSubMesh->AddVertex(
			FVector2D(FMath::Cos(sectionEnd) * radius.X + radius.X, FMath::Sin(sectionEnd) * radius.Y + radius.Y),
			LineColor);

		pSubMesh->AddTriangles(SECTOR_CENTER_TRIANGLES, 24, sides * 3 + 1);
	}
}

bool FEllipseMesh::HitTest(
	const FBox2D& ContentRect, const FVector2D& LocalPoint, float LineWidth, float StartDegree, float EndDegree)
{
	FVector2D Radius = ContentRect.GetSize() * 0.5f;
	FVector2D Pos = LocalPoint - Radius - ContentRect.Min;
	if (FMath::Pow(Pos.X / Radius.X, 2) + FMath::Pow(Pos.Y / Radius.Y, 2) < 1)
	{
		if (StartDegree != 0 || EndDegree != 360)
		{
			float deg = FMath::RadiansToDegrees(FMath::Atan2(Pos.Y, Pos.X));
			if (deg < 0)
				deg += 360;
			return deg >= StartDegree && deg <= EndDegree;
		}
		else
			return true;
	}

	return false;
}