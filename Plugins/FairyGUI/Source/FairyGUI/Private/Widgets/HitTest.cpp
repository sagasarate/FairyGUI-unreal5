#include "Widgets/HitTest.h"
#include "Utils/ByteBuffer.h"
#include "UI/GObject.h"
#include "UI/GImage.h"

void FPixelHitTestData::Load(FByteBuffer* Buffer)
{
	Buffer->Skip(4);
	PixelWidth = Buffer->ReadInt();
	Scale = 1.0f / Buffer->ReadByte();
	int32 PixelsLength = Buffer->ReadInt();
	Pixels.Append(Buffer->GetBuffer() + Buffer->GetPos(), PixelsLength);
}

FPixelHitTest::FPixelHitTest(const TSharedPtr<FPixelHitTestData>& InData, int32 InOffsetX, int32 InOffsetY,
	UGObject* pParent, UGImage* pSourceImage)
	: OffsetX(InOffsetX), OffsetY(InOffsetY), Data(InData), Parent(pParent), SourceImage(pSourceImage)
{
}

FPixelHitTest::~FPixelHitTest() {}

bool FPixelHitTest::HitTest(const FVector2D& GlobalPoint) const
{
	UGObject* pParent = Parent.Pin().Get();
	if (!pParent)
		return false;
	auto	  LocalPoint = pParent->GlobalToLocal(GlobalPoint);
	UGImage*  pImage = SourceImage.Pin().Get();
	FVector2D LayoutScaleMultiplier(1.0f, 1.0f);
	if (pImage)
	{
		LayoutScaleMultiplier = pImage->GetSize() / pImage->SourceSize;
		if (LayoutScaleMultiplier.ContainsNaN())
			LayoutScaleMultiplier.Set(1, 1);
	}

	int32 x = FMath::FloorToInt((LocalPoint.X / LayoutScaleMultiplier.X - OffsetX) * Data->Scale);
	int32 y = FMath::FloorToInt((LocalPoint.Y / LayoutScaleMultiplier.Y - OffsetY) * Data->Scale);
	if (x < 0 || y < 0 || x >= Data->PixelWidth)
		return false;

	int32 pos = y * Data->PixelWidth + x;
	int32 pos2 = pos / 8;
	int32 pos3 = pos % 8;

	if (pos2 >= 0 && pos2 < Data->Pixels.Num())
		return ((Data->Pixels[pos2] >> pos3) & 0x1) > 0;
	else
		return false;
}

FChildHitTest::FChildHitTest(UGObject* InObj) : Obj(InObj)
{
	if (InObj)
		InObj->GetDisplayObject()->SetForceUpdateGeometry(true);
}

FChildHitTest::~FChildHitTest() {}

bool FChildHitTest::HitTest(const FVector2D& GlobalPoint) const
{
	if (!Obj.IsValid() || Obj->GetParent() == nullptr)
		return false;

	auto DisplayObj = Obj->GetDisplayObject();
	return DisplayObj->HitTest(GlobalPoint) != nullptr;
}