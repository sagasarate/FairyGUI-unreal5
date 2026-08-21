#pragma once

#include "SDisplayObject.h"
#include "Utils/TypeID.h"
#include "Widgets/Mesh/SubMesh.h"

class FAIRYGUI_API SSelectionShape : public SDisplayObject
{
protected:
	TArray<FBox2D> m_Rects;
	bool		   m_MousePressed = false;

	mutable TSharedPtr<FSubMesh> m_pSubMesh;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMouseEVent, const FPointerEvent&);

	FOnMouseEVent OnClick;
	FOnMouseEVent OnRollover;
	FOnMouseEVent OnRollout;

public:
	DECLARE_TYPE_ID(SSelectionShape, SDisplayObject)

	explicit SSelectionShape(UGObject* InGObject = nullptr);

	void SetRects(const TArray<FBox2D>& Rects, const FColor& Color)
	{
		m_Rects = Rects;
		m_Color = Color;
		m_bRebuildMesh = true;
	}
	void AddRect(const FBox2D& Rect)
	{
		m_Rects.Add(Rect);
		m_bRebuildMesh = true;
	}
	void SetColor(const FColor& Color)
	{
		m_Color = Color;
		m_bRebuildMesh = true;
	}
	TArray<FBox2D>& GetRects() { return m_Rects; }
	void			Clear()
	{
		m_Rects.Reset();
		m_bRebuildMesh = true;
	}

public:
	virtual bool OnMouseButtonDown(const FPointerEvent& MouseEvent) override;
	virtual bool OnMouseButtonUp(const FPointerEvent& MouseEvent) override;
	virtual void   OnMouseEnter(const FPointerEvent& MouseEvent) override;
	virtual void   OnMouseLeave(const FPointerEvent& MouseEvent) override;

	virtual void CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
		float InSaturation) const override;

	virtual const SDisplayObject* HitTest(const FVector2D& GlobalPoint) const override;

protected:
	void RebuildMesh(int32 LayerID);

public:
	static TArray<TSharedPtr<SSelectionShape>> m_Pool;

	static TSharedPtr<SSelectionShape> Borrow();

	static void Return(TSharedPtr<SSelectionShape> Value);
	static void Return(TArray<TSharedPtr<SSelectionShape>>& Values);
};