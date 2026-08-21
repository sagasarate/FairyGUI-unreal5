#pragma once

#include "Widgets/STextField.h"
#include "Font/NTextFormat.h"
#include "Utils/HTML/HTMLElement.h"
#include "Utils/HTML/HTMLParseOptions.h"
#include "Utils/TypeID.h"

class UBaseFont;

class FAIRYGUI_API SRichTextField : public STextField
{
public:
	struct Emoji
	{
		FString Url;
		int		Width = 0;
		int		Height = 0;
		Emoji() {}
		Emoji(const FString& url, int width, int height)
		{
			Url = url;
			Width = width;
			Height = height;
		}

		Emoji(FString url) { Url = url; }
	};

protected:
	TMap<uint32, Emoji>					 m_Emojies;
	TArray<TSharedRef<SDisplayObject>>	 m_Children;
	int32								 m_BackCount = 0; // ponytail: m_Children[0, m_BackCount) 为 Back 层，其余为 Front 层

public:
	FHTMLParseOptions HTMLParseOptions;

public:
	DECLARE_TYPE_ID(SRichTextField, STextField)
	explicit SRichTextField(UGObject* InGObject = nullptr);

	bool				 HaveEmoji() { return m_Emojies.Num() != 0; }
	TMap<uint32, Emoji>& GetEmojis() { return m_Emojies; }

	bool AddChild(const TSharedRef<SDisplayObject>& SlotWidget, EDisplayObjChildLayer Layer = EDisplayObjChildLayer::Front);
	bool AddChildAt(
		const TSharedRef<SDisplayObject>& SlotWidget, int32 Index, EDisplayObjChildLayer Layer = EDisplayObjChildLayer::Front);
	int32 GetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget) const;
	TSharedPtr<SDisplayObject> GetChildAt(int32 Index) const;
	bool  SetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index, EDisplayObjChildLayer Layer);
	void  RemoveChild(const TSharedRef<SDisplayObject>& SlotWidget);
	void  RemoveChildAt(int32 Index);
	void  RemoveChildren(int32 BeginIndex = 0, int32 EndIndex = -1);
	int32 NumChildren() const;

public:
	// virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	// 	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	// 	bool bParentEnabled) const override;

protected:
	virtual void CollectRenderUnits(
		FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha, float InSaturation) const override;
	virtual const SDisplayObject* HitTest(const FVector2D& GlobalPoint) const override;
};