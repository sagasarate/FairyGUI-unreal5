#pragma once

#include "SDisplayObject.h"
#include "Utils/TypeID.h"

class FAIRYGUI_API SContainer : public SDisplayObject
{
protected:
	TArray<TSharedRef<SDisplayObject>>	m_Children;
	TSharedPtr<SDisplayObject>			m_Mask;
	bool								m_IsMaskReversed = false;
	bool								m_bTabStopScope = false;

public:
	DECLARE_TYPE_ID(SContainer, SDisplayObject)

	explicit SContainer(UGObject* InGObject = nullptr);

	void  AddChild(const TSharedRef<SDisplayObject>& SlotWidget);
	void  AddChildAt(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index);
	int32 GetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget) const;
	TSharedPtr<SDisplayObject> GetChildAt(int32 Index) const;
	void  SetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index);
	void  RemoveChild(const TSharedRef<SDisplayObject>& SlotWidget);
	void  RemoveChildAt(int32 Index);
	void  RemoveChildren(int32 BeginIndex = 0, int32 EndIndex = -1);
	int32 NumChildren() const;

	// Tab 导航是否限定在此容器范围内
	bool IsTabStopScope() const { return m_bTabStopScope; }
	void SetTabStopScope(bool bEnable) { m_bTabStopScope = bEnable; }

	void SetMask(const TSharedPtr<SDisplayObject>& Mask, bool bReversed = false)
	{
		if (m_Mask.IsValid())
		{
			m_Mask->SetForceUpdateGeometry(false);
		}
		m_Mask = Mask;
		m_IsMaskReversed = bReversed;
		if (m_Mask.IsValid())
		{
			m_Mask->SetInteractable(false);
			m_Mask->SetVisible(false);
			m_Mask->SetForceUpdateGeometry(true);
		}
		EnableHitTest(m_Mask.IsValid());
	}
	TSharedPtr<SDisplayObject> GetMask() const { return m_Mask; }
	bool					   IsMaskReversed() const { return m_IsMaskReversed; }

public:
	virtual void CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
			  float InSaturation) const override;

	virtual const SDisplayObject* HitTest(const FVector2D& GlobalPoint) const override;

protected:
};
