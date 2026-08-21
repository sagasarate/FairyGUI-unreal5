#include "Widgets/Mesh/RoundedRectMesh.h"

void FRoundedRectMesh::BuildMesh(FSubMesh* pSubMesh, float LineWidth, const FColor& LineColor,
	const FColor* pCenterColor, float TopLeftRadius, float TopRightRadius, float BottomLeftRadius,
	float BottomRightRadius)
{
	const FBox2D& rect = pSubMesh->ContentRect;
	const FColor& color = pSubMesh->VertexColor;
	int32		  StartIndex = pSubMesh->Vertices.Num();

	FVector2D radius = rect.GetSize() * 0.5f;
	float	  cornerMaxRadius = radius.GetMin();
	FVector2D center = radius + rect.Min;

	bool bDrawRect = color.A != 0;

	pSubMesh->AddVertex(center, pCenterColor ? *pCenterColor : color);
	int32 cnt = pSubMesh->Vertices.Num() - StartIndex;
	for (int32 i = 0; i < 4; i++)
	{
		float cornerRadius = 0;
		switch (i)
		{
			case 0:
				cornerRadius = BottomRightRadius;
				break;

			case 1:
				cornerRadius = BottomLeftRadius;
				break;

			case 2:
				cornerRadius = TopLeftRadius;
				break;

			case 3:
				cornerRadius = TopRightRadius;
				break;
		}
		cornerRadius = FMath::Min(cornerMaxRadius, cornerRadius);

		FVector2D offset = rect.Min;

		if (i == 0 || i == 3)
			offset.X = rect.Max.X - cornerRadius * 2;
		if (i == 0 || i == 1)
			offset.Y = rect.Max.Y - cornerRadius * 2;

		if (cornerRadius != 0)
		{
			int32 partNumSides = FMath::Max(1, FMath::CeilToInt(PI * cornerRadius / 8)) + 1;
			float angleDelta = PI / 2 / partNumSides;
			float angle = PI / 2 * i;
			float startAngle = angle;

			for (int32 j = 1; j <= partNumSides; j++)
			{
				if (j == partNumSides)
					angle = startAngle + PI / 2;
				FVector2D v1(offset.X + FMath::Cos(angle) * (cornerRadius - LineWidth) + cornerRadius,
					offset.Y + FMath::Sin(angle) * (cornerRadius - LineWidth) + cornerRadius);
				pSubMesh->AddVertex(v1, color);
				if (LineWidth != 0)
				{
					pSubMesh->AddVertex(v1, LineColor);
					pSubMesh->AddVertex(FVector2D(offset.X + FMath::Cos(angle) * cornerRadius + cornerRadius,
											offset.Y + FMath::Sin(angle) * cornerRadius + cornerRadius),
						LineColor);
				}
				angle += angleDelta;
			}
		}
		else
		{
			FVector2D v1 = offset;
			if (LineWidth != 0)
			{
				if (i == 0 || i == 3)
					offset.X -= LineWidth;
				else
					offset.X += LineWidth;
				if (i == 0 || i == 1)
					offset.Y -= LineWidth;
				else
					offset.Y += LineWidth;
				pSubMesh->AddVertex(offset, color);
				pSubMesh->AddVertex(offset, LineColor);
				pSubMesh->AddVertex(v1, LineColor);
			}
			else
				pSubMesh->AddVertex(v1, color);
		}
	}
	cnt = pSubMesh->Vertices.Num() - StartIndex - cnt;

	if (LineWidth > 0)
	{
		for (int32 i = 0; i < cnt; i += 3)
		{
			if (i != cnt - 3)
			{
				if (bDrawRect)
					pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + i + 4);
				pSubMesh->AddTriangle(StartIndex + i + 5, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + i + 6, StartIndex + i + 5);
			}
			else
			{
				if (bDrawRect)
					pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, StartIndex + 1);
				pSubMesh->AddTriangle(StartIndex + 2, StartIndex + i + 2, StartIndex + i + 3);
				pSubMesh->AddTriangle(StartIndex + i + 3, StartIndex + 3, StartIndex + 2);
			}
		}
	}
	else if (bDrawRect)
	{
		for (int32 i = 0; i < cnt; i++)
			pSubMesh->AddTriangle(StartIndex, StartIndex + i + 1, (i == cnt - 1) ? StartIndex + 1 : StartIndex + i + 2);
	}
}

bool FRoundedRectMesh::HitTest(const FBox2D& ContentRect, const FVector2D& LocalPoint, float TopLeftRadius,
	float TopRightRadius, float BottomLeftRadius, float BottomRightRadius)
{
	if (!ContentRect.IsInside(LocalPoint))
		return false;

	FVector2D radius = ContentRect.GetSize() * 0.5f;
	float	  cornerMaxRadius = radius.GetMin();

	auto TestCorner = [&](float CornerRadius, const FVector2D& CornerCenter) {
		if (CornerRadius <= 0)
			return true;
		CornerRadius = FMath::Min(cornerMaxRadius, CornerRadius);
		FVector2D d = LocalPoint - CornerCenter;
		return d.X * d.X + d.Y * d.Y <= CornerRadius * CornerRadius;
	};

	if (LocalPoint.X < ContentRect.Min.X + TopLeftRadius && LocalPoint.Y < ContentRect.Min.Y + TopLeftRadius)
		return TestCorner(
			TopLeftRadius, FVector2D(ContentRect.Min.X + TopLeftRadius, ContentRect.Min.Y + TopLeftRadius));

	if (LocalPoint.X > ContentRect.Max.X - TopRightRadius && LocalPoint.Y < ContentRect.Min.Y + TopRightRadius)
		return TestCorner(
			TopRightRadius, FVector2D(ContentRect.Max.X - TopRightRadius, ContentRect.Min.Y + TopRightRadius));

	if (LocalPoint.X < ContentRect.Min.X + BottomLeftRadius && LocalPoint.Y > ContentRect.Max.Y - BottomLeftRadius)
		return TestCorner(
			BottomLeftRadius, FVector2D(ContentRect.Min.X + BottomLeftRadius, ContentRect.Max.Y - BottomLeftRadius));

	if (LocalPoint.X > ContentRect.Max.X - BottomRightRadius && LocalPoint.Y > ContentRect.Max.Y - BottomRightRadius)
		return TestCorner(
			BottomRightRadius, FVector2D(ContentRect.Max.X - BottomRightRadius, ContentRect.Max.Y - BottomRightRadius));

	return true;
}
