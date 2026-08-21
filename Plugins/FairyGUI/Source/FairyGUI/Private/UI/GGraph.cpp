#include "UI/GGraph.h"
#include "Utils/ByteBuffer.h"
#include "Widgets/NTexture.h"
#include "Widgets/SShape.h"

UGGraph::UGGraph() {}

void UGGraph::CreateDisplayObject()
{
	DisplayObject = Content = MakeShared<SShape>(this);
}

UGGraph::~UGGraph() {}

void UGGraph::Dispose()
{
	if (bDisposed)
		return;
	Content.Reset();
	UGObject::Dispose();
}

const FColor& UGGraph::GetColor() const
{
	return Content->GetColor();
}

void UGGraph::SetColor(const FColor& InColor)
{
	Content->SetColor(InColor);
}

void UGGraph::DrawRect(float LineWidth, const FColor& LineColor, const FColor& FillColor)
{
	Content->SetType(EFGUIShapeType::Rect);
	Content->SetLineWidth(LineWidth);
	Content->SetLineColor(LineColor);
	Content->SetColor(FillColor);
}

void UGGraph::DrawRoundRect(float LineWidth, const FColor& LineColor, const FColor& FillColor, float TopLeftRadius,
	float TopRightRadius, float BottomLeftRadius, float BottomRightRadius)
{
	Content->SetType(EFGUIShapeType::RoundRect);
	Content->SetLineWidth(LineWidth);
	Content->SetLineColor(LineColor);
	Content->SetColor(FillColor);
	Content->SetTopLeftRadius(TopLeftRadius);
	Content->SetTopRightRadius(TopRightRadius);
	Content->SetBottomLeftRadius(BottomLeftRadius);
	Content->SetBottomRightRadius(BottomRightRadius);
}

void UGGraph::DrawEllipse(
	float LineWidth, const FColor& LineColor, const FColor& FillColor, float StartDegree, float EndDegree)
{
	Content->SetType(EFGUIShapeType::Ellipse);
	Content->SetLineWidth(LineWidth);
	Content->SetLineColor(LineColor);
	Content->SetColor(FillColor);
	Content->SetStartDegree(StartDegree);
	Content->SetEndDegree(EndDegree);
}

void UGGraph::DrawPolygon(
	float LineWidth, const FColor& LineColor, const FColor& FillColor, const TArray<FVector2D>& Points)
{
	Content->SetType(EFGUIShapeType::Polygon);
	Content->SetLineWidth(LineWidth);
	Content->SetLineColor(LineColor);
	Content->SetColor(FillColor);
	Content->SetPolygonPoints(Points);
}

void UGGraph::DrawRegularPolygon(int32 Sides, float LineWidth, const FColor& LineColor, const FColor& FillColor,
	float ShapeRotation, const TArray<float>& Distances)
{
	Content->SetType(EFGUIShapeType::RegularPolygon);
	Content->SetLineWidth(LineWidth);
	Content->SetLineColor(LineColor);
	Content->SetColor(FillColor);
	Content->SetPolygonSides(Sides);
	Content->SetPolygonRotation(ShapeRotation);
	Content->SetPolygonDistances(Distances);
}

void UGGraph::Clear()
{
	Content->SetType(EFGUIShapeType::None);
}

bool UGGraph::IsEmpty() const
{
	return Content->GetType() == EFGUIShapeType::None;
}

EFGUIShapeType UGGraph::GetType() const
{
	return Content->GetType();
}

void UGGraph::SetType(EFGUIShapeType InShapeType)
{
	Content->SetType(InShapeType);
}

float UGGraph::GetLineWidth() const
{
	return Content->GetLineWidth();
}

void UGGraph::SetLineWidth(float InLineWidth)
{
	Content->SetLineWidth(InLineWidth);
}

FColor UGGraph::GetLineColor() const
{
	return Content->GetLineColor();
}

void UGGraph::SetLineColor(const FColor& InLineColor)
{
	Content->SetLineColor(InLineColor);
}

bool UGGraph::HaveCenterColor() const
{
	return Content->HaveCenterColor();
}

FColor UGGraph::GetCenterColor() const
{
	return Content->GetCenterColor();
}

void UGGraph::SetCenterColor(const FColor& CenterColor)
{
	Content->SetCenterColor(&CenterColor);
}

void UGGraph::ClearCenterColor()
{
	Content->SetCenterColor(nullptr);
}

const TArray<FColor>& UGGraph::GetColors() const
{
	return Content->GetColors();
}

void UGGraph::SetColors(const TArray<FColor>& InColors)
{
	Content->SetColors(InColors);
}

float UGGraph::GetTopLeftRadius() const
{
	return Content->GetTopLeftRadius();
}

void UGGraph::SetTopLeftRadius(float InRadius)
{
	Content->SetTopLeftRadius(InRadius);
}

float UGGraph::GetTopRightRadius() const
{
	return Content->GetTopRightRadius();
}

void UGGraph::SetTopRightRadius(float InRadius)
{
	Content->SetTopRightRadius(InRadius);
}

float UGGraph::GetBottomLeftRadius() const
{
	return Content->GetBottomLeftRadius();
}

void UGGraph::SetBottomLeftRadius(float InRadius)
{
	Content->SetBottomLeftRadius(InRadius);
}

float UGGraph::GetBottomRightRadius() const
{
	return Content->GetBottomRightRadius();
}

void UGGraph::SetBottomRightRadius(float InRadius)
{
	Content->SetBottomRightRadius(InRadius);
}

float UGGraph::GetStartDegree() const
{
	return Content->GetStartDegree();
}

void UGGraph::SetStartDegree(float InDegree)
{
	Content->SetStartDegree(InDegree);
}

float UGGraph::GetEndDegree() const
{
	return Content->GetEndDegree();
}

void UGGraph::SetEndDegree(float InDegree)
{
	Content->SetEndDegree(InDegree);
}

const TArray<FVector2D>& UGGraph::GetPolygonPoints() const
{
	return Content->GetPolygonPoints();
}

void UGGraph::SetPolygonPoints(const TArray<FVector2D>& InPoints)
{
	Content->SetPolygonPoints(InPoints);
}

bool UGGraph::GetUsePercentPositions() const
{
	return Content->GetUsePercentPositions();
}

void UGGraph::SetUsePercentPositions(bool bInUsePercentPositions)
{
	Content->SetUsePercentPositions(bInUsePercentPositions);
}

int32 UGGraph::GetPolygonSides() const
{
	return Content->GetPolygonSides();
}

void UGGraph::SetPolygonSides(int32 InSides)
{
	Content->SetPolygonSides(InSides);
}

float UGGraph::GetPolygonRotation() const
{
	return Content->GetPolygonRotation();
}

void UGGraph::SetPolygonRotation(float InRotation)
{
	Content->SetPolygonRotation(InRotation);
}

const TArray<float>& UGGraph::GetPolygonDistances() const
{
	return Content->GetPolygonDistances();
}

void UGGraph::SetPolygonDistances(const TArray<float>& InDistances)
{
	Content->SetPolygonDistances(InDistances);
}

UNTexture* UGGraph::GetTexture() const
{
	return Content->GetTexture();
}

void UGGraph::SetTexture(UNTexture* InTexture)
{
	Content->SetTexture(InTexture);
}

const TArray<FVector2D>& UGGraph::GetTexcoords() const
{
	return Content->GetTexcoords();
}

void UGGraph::SetTexcoords(const TArray<FVector2D>& InTexcoords)
{
	Content->SetTexcoords(InTexcoords);
}

FNVariant UGGraph::GetProp(EObjectPropID PropID) const
{
	switch (PropID)
	{
		case EObjectPropID::Color:
			return FNVariant(Content->GetColor());
		default:
			return UGObject::GetProp(PropID);
	}
}

void UGGraph::SetProp(EObjectPropID PropID, const FNVariant& InValue)
{
	switch (PropID)
	{
		case EObjectPropID::Color:
			SetColor(InValue.AsColor());
			break;
		default:
			UGObject::SetProp(PropID, InValue);
			break;
	}
}

void UGGraph::SetupBeforeAdd(FByteBuffer* Buffer, int32 BeginPos)
{
	UGObject::SetupBeforeAdd(Buffer, BeginPos);

	Buffer->Seek(BeginPos, 5);

	int32 type = Buffer->ReadByte();
	if (type != 0)
	{
		int32	 lineWidth = Buffer->ReadInt();
		FColor	 lineColor = Buffer->ReadColor();
		FColor	 fillColor = Buffer->ReadColor();
		bool	 roundedRect = Buffer->ReadBool();
		FVector4 cornerRadius;
		if (roundedRect)
		{
			for (int32 i = 0; i < 4; i++)
				cornerRadius[i] = Buffer->ReadFloat();
		}

		if (type == 1)
		{
			if (roundedRect)
				DrawRoundRect(
					lineWidth, lineColor, fillColor, cornerRadius.X, cornerRadius.Y, cornerRadius.Z, cornerRadius.W);
			else
				DrawRect(lineWidth, lineColor, fillColor);
		}
		else if (type == 2)
			DrawEllipse(lineWidth, lineColor, fillColor);
		else if (type == 3)
		{
			int32			  cnt = Buffer->ReadShort() / 2;
			TArray<FVector2D> points;
			for (int32 i = 0; i < cnt; i++)
			{
				float f1 = Buffer->ReadFloat();
				float f2 = Buffer->ReadFloat();
				points.Add(FVector2D(f1, f2));
			}

			DrawPolygon(lineWidth, lineColor, fillColor, points);
		}
		else if (type == 4)
		{
			int32		  sides = Buffer->ReadShort();
			float		  startAngle = Buffer->ReadFloat();
			int32		  cnt = Buffer->ReadShort();
			TArray<float> distances;
			if (cnt > 0)
			{
				for (int32 i = 0; i < cnt; i++)
					distances.Add(Buffer->ReadFloat());
			}

			DrawRegularPolygon(sides, lineWidth, lineColor, fillColor, startAngle, distances);
		}
	}
}