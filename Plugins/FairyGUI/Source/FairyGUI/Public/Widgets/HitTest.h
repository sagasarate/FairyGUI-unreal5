#pragma once

#include "CoreMinimal.h"

class UGObject;
class UGImage;

class FAIRYGUI_API IHitTest
{
public:
	virtual bool HitTest(const FVector2D& GlobalPoint) const = 0;
};

struct FAIRYGUI_API FPixelHitTestData
{
public:
	int32		  PixelWidth;
	float		  Scale;
	TArray<uint8> Pixels;

	void Load(class FByteBuffer* Buffer);
};

class FAIRYGUI_API FPixelHitTest : public IHitTest
{
public:
	FPixelHitTest(const TSharedPtr<FPixelHitTestData>& Data, int32 OffsetX, int32 OffsetY, UGObject* pParent,
		UGImage* pSourceImage);
	virtual ~FPixelHitTest();

	virtual bool HitTest(const FVector2D& GlobalPoint) const override;

	int32 OffsetX;
	int32 OffsetY;

private:
	TSharedPtr<FPixelHitTestData> Data;
	TWeakObjectPtr<UGObject>	  Parent;
	TWeakObjectPtr<UGImage>		  SourceImage;
};

class FAIRYGUI_API FChildHitTest : public IHitTest
{
public:
	FChildHitTest(UGObject* InObj);
	virtual ~FChildHitTest();

	virtual bool HitTest(const FVector2D& GlobalPoint) const override;

	TWeakObjectPtr<UGObject> Obj;
};