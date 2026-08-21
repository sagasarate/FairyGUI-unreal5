
#include "Widgets/SContainer.h"
#include "FairyApplication.h"
#include "UI/GObject.h"

IMPLEMENT_TYPE_ID(SContainer)

SContainer::SContainer(UGObject* InGObject) : SDisplayObject(InGObject) {}

void SContainer::AddChild(const TSharedRef<SDisplayObject>& SlotWidget)
{
	AddChildAt(SlotWidget, m_Children.Num());
}

void SContainer::AddChildAt(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index)
{
	int32 Count = m_Children.Num();
	verifyf(Index >= 0 && Index <= Count, TEXT("Invalid child index"));

	if (SlotWidget->GetParent().Get() == this)
		SetChildIndex(SlotWidget, Index);
	else
	{
		verifyf(!SlotWidget->GetParent().IsValid(), TEXT("Cant add a child has parent"));

		m_Children.Insert(SlotWidget, Index);
		SlotWidget->SetParent(SharedThis(this));

		UGObject* OnStageObj = SDisplayObject::GetGObjectIfOnStage(SharedThis(this));
		if (OnStageObj != nullptr)
		{
			OnStageObj->GetApp()->BroadcastEvent(FUIEvents::AddedToStage, SlotWidget);
		}
	}
}

void SContainer::SetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index)
{
	int32 OldIndex = GetChildIndex(SlotWidget);
	verifyf(OldIndex != -1, TEXT("Not a child of this container"));
	if (OldIndex == Index)
		return;
	TSharedRef<SDisplayObject> Widget = m_Children[OldIndex];
	m_Children.RemoveAt(OldIndex);
	// RemoveAt 之后 clamp，而非之前，避免 OldIndex<Index 时回退过头
	Index = FMath::Clamp(Index, 0, m_Children.Num());
	m_Children.Insert(Widget, Index);
}

void SContainer::RemoveChild(const TSharedRef<SDisplayObject>& SlotWidget)
{
	int32 Index = GetChildIndex(SlotWidget);
	RemoveChildAt(Index);
}

void SContainer::RemoveChildAt(int32 Index)
{
	verifyf(Index >= 0 && Index < m_Children.Num(), TEXT("Invalid child index"));
	TSharedRef<SDisplayObject> SlotWidget = m_Children[Index];

	UGObject* OnStageObj = SDisplayObject::GetGObjectIfOnStage(SharedThis(this));
	if (OnStageObj != nullptr)
	{
		OnStageObj->GetApp()->BroadcastEvent(FUIEvents::RemovedFromStage, SlotWidget);
	}

	// ponytail: HitTestGrid::RemoveWidget 已移除，SDisplayObject 不再注册到 Slate HitTestGrid

	SlotWidget->SetParent(nullptr);
	m_Children.RemoveAt(Index);
}

int32 SContainer::GetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget) const
{
	return m_Children.IndexOfByPredicate([&](const TSharedRef<SDisplayObject>& Child) { return Child == SlotWidget; });
}

TSharedPtr<SDisplayObject> SContainer::GetChildAt(int32 Index) const
{
	if (Index >= 0 && Index < m_Children.Num())
		return m_Children[Index];
	return nullptr;
}

void SContainer::RemoveChildren(int32 BeginIndex, int32 EndIndex)
{
	if (EndIndex < 0 || EndIndex >= m_Children.Num())
		EndIndex = m_Children.Num() - 1;

	UGObject*		   OnStageObj = SDisplayObject::GetGObjectIfOnStage(SharedThis(this));
	UFairyApplication* Dispatcher = OnStageObj != nullptr ? OnStageObj->GetApp() : nullptr;

	if (Dispatcher != nullptr || BeginIndex > 0 || EndIndex < m_Children.Num() - 1)
	{
		for (int32 i = BeginIndex; i <= EndIndex; ++i)
		{
			TSharedRef<SDisplayObject> SlotWidget = m_Children[BeginIndex];
			if (Dispatcher != nullptr)
			{
				Dispatcher->BroadcastEvent(FUIEvents::RemovedFromStage, SlotWidget);
			}
			// ponytail: HitTestGrid::RemoveWidget 已移除
			SlotWidget->SetParent(nullptr);
			m_Children.RemoveAt(BeginIndex);
		}
	}
	else
		m_Children.Empty();
}

int32 SContainer::NumChildren() const
{
	return m_Children.Num();
}

void SContainer::CollectRenderUnits(
	FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha, float InSaturation) const
{
	FRenderGroup* CurrentGroup = ParentGroup;

	// === 1. 裁剪 / 遮罩组（合并为单一中间组）===
	bool bNeedClip = m_bClipToBounds;
	bool bNeedMask = m_Mask.IsValid();

	if (bNeedClip || bNeedMask)
	{
		// 裁剪-遮罩合并：只创建一个 FRenderGroup，同时承载 clip 和 mask 属性
		auto pGroup = FRenderGroup::Borrow();
		if (!pGroup)
			return;

		if (bNeedClip)
		{
			FSlateRect FloatRect = AllottedGeometry.GetRenderBoundingRect();
			// 应用裁剪边界扩展，防止溢出内容被错误剔除
			FloatRect.Left -= m_CullingBoundsExtension.Left;
			FloatRect.Top -= m_CullingBoundsExtension.Top;
			FloatRect.Right += m_CullingBoundsExtension.Right;
			FloatRect.Bottom += m_CullingBoundsExtension.Bottom;

			FIntRect ClipRect(FMath::FloorToInt(FloatRect.Left), FMath::FloorToInt(FloatRect.Top),
				FMath::CeilToInt(FloatRect.Right), FMath::CeilToInt(FloatRect.Bottom));
			if (ParentGroup->IsOutsize(ClipRect))
			{
				FRenderGroup::Return(pGroup);
				return;
			}
			// 与父组裁剪范围取交集，反映祖先累积的真实裁剪约束
			ClipRect = FIntRect(FMath::Max(ClipRect.Min.X, ParentGroup->ClippingRect.Min.X),
				FMath::Max(ClipRect.Min.Y, ParentGroup->ClippingRect.Min.Y),
				FMath::Min(ClipRect.Max.X, ParentGroup->ClippingRect.Max.X),
				FMath::Min(ClipRect.Max.Y, ParentGroup->ClippingRect.Max.Y));
			// 交集后整数尺寸≤0则整组不渲染
			if (ClipRect.Min.X >= ClipRect.Max.X || ClipRect.Min.Y >= ClipRect.Max.Y)
			{
				FRenderGroup::Return(pGroup);
				return;
			}
			pGroup->bHasClipping = true;
			pGroup->ClippingRect = ClipRect;
			pGroup->WorldBounds =
				FBox2D(FVector2D(ClipRect.Min.X, ClipRect.Min.Y), FVector2D(ClipRect.Max.X, ClipRect.Max.Y));
		}

		if (bNeedMask)
		{
			pGroup->bHasMask = true;
			pGroup->bReversedMask = m_IsMaskReversed;

			// 收集遮罩对象的渲染单元（进入 Children 前部）
			int32 ChildCountBefore = pGroup->Children.Num();
			m_Mask->CaculateGeometry(AllottedGeometry);
			m_Mask->CollectRenderUnits(pGroup, m_Mask->GetPaintSpaceGeometry(), InAlpha * m_Mask->GetRenderOpacity(),
				InSaturation * m_Mask->GetSaturation());
			int32 MaskUnitCount = pGroup->Children.Num() - ChildCountBefore;
			pGroup->MaskChildCount = MaskUnitCount;
		}

		// 若 pGroup 没有自己的裁剪约束（mask-only 组），则继承父组的裁剪范围，
		// 确保后续子元素的 IsOutsize 检查能感知祖先裁剪，防止不相交的 scissor rect
		if (!pGroup->bHasClipping)
			pGroup->ClippingRect = ParentGroup->ClippingRect;

		ParentGroup->AddChild(pGroup);
		CurrentGroup = pGroup;
	}

	// === 3. 遍历子元素 ===
	for (int32 ChildIndex = 0; ChildIndex < m_Children.Num(); ++ChildIndex)
	{
		const TSharedRef<SDisplayObject>& CurChild = m_Children[ChildIndex];
		const SDisplayObject&			  DisplayObj = CurChild.Get();
		if (DisplayObj.IsVisible())
		{
			DisplayObj.CaculateGeometry(AllottedGeometry);
			DisplayObj.CollectRenderUnits(CurrentGroup, DisplayObj.GetPaintSpaceGeometry(),
				InAlpha * DisplayObj.GetRenderOpacity(), InSaturation * DisplayObj.GetSaturation());
		}
		else if (DisplayObj.IsForceUpdateGeometry())
		{
			DisplayObj.CaculateGeometry(AllottedGeometry);
		}
	}
	m_AttrChangeFlag = 0;
}

const SDisplayObject* SContainer::HitTest(const FVector2D& GlobalPoint) const
{
	if (m_Mask.IsValid())
	{
		bool Result = m_Mask->HitTest(GlobalPoint) != nullptr;
		if (m_IsMaskReversed)
			Result = !Result;
		if (!Result)
			return nullptr;
	}

	if (GObject.IsValid())
	{
		auto HitArea = GObject->GetHitArea();
		if (HitArea)
		{
			if (!HitArea->HitTest(GlobalPoint))
				return nullptr;
		}
	}

	FBox2D ContentRect(FVector2D::ZeroVector, m_Size);
	auto   LocalPoint = m_CachedInverseDesktopTransform.TransformPoint(GlobalPoint);
	bool   IsInContentRect = ContentRect.IsInside(LocalPoint);

	if (m_bClipToBounds && !IsInContentRect)
		return nullptr;

	// 从后向前遍历（高索引在上层，先命中），匹配 Slate 的 z-order
	for (int32 ChildIndex = m_Children.Num() - 1; ChildIndex >= 0; --ChildIndex)
	{
		const TSharedRef<SDisplayObject>& CurChild = m_Children[ChildIndex];
		if (CurChild->IsVisible() && CurChild->IsHitTestVisible())
		{
			auto Result = CurChild->HitTest(GlobalPoint);
			if (Result)
				return Result;
		}
	}

	if (m_bOpaque && IsInContentRect)
		return this;
	return nullptr;
}
