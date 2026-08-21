#include "Widgets/SRichTextField.h"
#include "FairyCommons.h"
#include "UI/GObject.h"

IMPLEMENT_TYPE_ID(SRichTextField)

SRichTextField::SRichTextField(UGObject* InGObject)
	: STextField(InGObject)
{
	SetInteractable(true);
}

bool SRichTextField::AddChild(const TSharedRef<SDisplayObject>& SlotWidget, EDisplayObjChildLayer Layer)
{
	return AddChildAt(SlotWidget, m_Children.Num(), Layer);
}

bool SRichTextField::AddChildAt(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index, EDisplayObjChildLayer Layer)
{
	if (SlotWidget->GetParent().Get() == this)
	{
		return SetChildIndex(SlotWidget, Index, Layer);
	}
	else
	{
		if (SlotWidget->GetParent().IsValid())
		{
			UE_LOG(LogFairyGUI, Error, TEXT("Cant add a child has parent"));
			return false;
		}

		int32 Count = m_Children.Num();
		Index = FMath::Clamp(Index, 0, Count);

		// ponytail: BackCount 替代 FDisplayObjChildSlot::Layer
		if (Layer == EDisplayObjChildLayer::Back)
		{
			Index = FMath::Min(Index, m_BackCount);
			m_Children.Insert(SlotWidget, Index);
			m_BackCount++;
		}
		else
		{
			Index = FMath::Max(Index, m_BackCount);
			m_Children.Insert(SlotWidget, Index);
		}
		SlotWidget->SetParent(StaticCastSharedRef<SDisplayObject>(AsShared()));
		return true;
	}
}

bool SRichTextField::SetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget, int32 Index, EDisplayObjChildLayer Layer)
{
	int32 OldIndex = GetChildIndex(SlotWidget);
	if (OldIndex == INDEX_NONE)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("Not a child of this container"));
		return false;
	}

	bool bWasBack = OldIndex < m_BackCount;
	bool bIsBack = (Layer == EDisplayObjChildLayer::Back);

	// 按层级约束调整目标索引
	if (bIsBack)
		Index = FMath::Min(Index, m_BackCount);
	else
		Index = FMath::Max(Index, m_BackCount);

	// 仅当位置和层级都没变时才可提前返回
	if (OldIndex == Index && bWasBack == bIsBack)
		return true;

	// Remove：若移除的是 Back 子节点，BackCount 减 1
	TSharedRef<SDisplayObject> Widget = m_Children[OldIndex];
	m_Children.RemoveAt(OldIndex);
	if (bWasBack)
		m_BackCount--;

	// 用 Remove 后的真实 BackCount 做层级约束
	if (bIsBack)
		Index = FMath::Min(Index, m_BackCount);
	else
		Index = FMath::Max(Index, m_BackCount);
	Index = FMath::Clamp(Index, 0, m_Children.Num());

	// Insert：若插入的是 Back 子节点，BackCount 加 1
	m_Children.Insert(Widget, Index);
	if (bIsBack)
		m_BackCount++;

	return true;
}

void SRichTextField::RemoveChild(const TSharedRef<SDisplayObject>& SlotWidget)
{
	int32 Index = GetChildIndex(SlotWidget);
	if (Index != INDEX_NONE)
		RemoveChildAt(Index);
}

void SRichTextField::RemoveChildAt(int32 Index)
{
	verifyf(Index >= 0 && Index < m_Children.Num(), TEXT("Invalid child index"));
	if (Index < m_BackCount)
		m_BackCount--;
	m_Children[Index]->SetParent(nullptr);
	m_Children.RemoveAt(Index);
}

int32 SRichTextField::GetChildIndex(const TSharedRef<SDisplayObject>& SlotWidget) const
{
	return m_Children.IndexOfByPredicate(
		[&](const TSharedRef<SDisplayObject>& Child) { return Child == SlotWidget; });
}

TSharedPtr<SDisplayObject> SRichTextField::GetChildAt(int32 Index) const
{
	if (Index >= 0 && Index < m_Children.Num())
		return m_Children[Index];
	return nullptr;
}

void SRichTextField::RemoveChildren(int32 BeginIndex, int32 EndIndex)
{
	if (EndIndex < 0 || EndIndex >= m_Children.Num())
		EndIndex = m_Children.Num() - 1;

	if (BeginIndex > 0 || EndIndex < m_Children.Num() - 1)
	{
		for (int32 i = BeginIndex; i <= EndIndex; ++i)
		{
			if (BeginIndex < m_BackCount)
				m_BackCount--;
			m_Children[BeginIndex]->SetParent(nullptr);
			m_Children.RemoveAt(BeginIndex);
		}
	}
	else
	{
		m_Children.Empty();
		m_BackCount = 0;
	}
}

int32 SRichTextField::NumChildren() const
{
	return m_Children.Num();
}

void SRichTextField::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
	float InSaturation) const
{
	FSlateRect FloatRect = AllottedGeometry.GetRenderBoundingRect();
	FIntRect   BoundingRect(FMath::FloorToInt(FloatRect.Left), FMath::FloorToInt(FloatRect.Top),
		  FMath::CeilToInt(FloatRect.Right), FMath::CeilToInt(FloatRect.Bottom));
	if (ParentGroup->IsOutsize(BoundingRect))
		return;

	TSharedPtr<FSlateFontCache> FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
	if (!FontCache.IsValid())
		return;


	const_cast<SRichTextField*>(this)->BuildLines();

	TArray<TSharedPtr<FTextSubMesh>> SubMeshs;
	m_TextMesh.GetSubMeshs(SubMeshs);

	FMatrix44f LocalToWorld = FMatrix44f(AllottedGeometry.GetAccumulatedRenderTransform().To3DMatrix());

	FRenderGroup* CurrentGroup = ParentGroup;
	if (m_ClipType == ETextClipType::ByPixel)
	{
		// 与父组裁剪范围取交集，反映祖先累积的真实裁剪约束
		FIntRect ClipRect = FIntRect(
			FMath::Max(BoundingRect.Min.X, ParentGroup->ClippingRect.Min.X),
			FMath::Max(BoundingRect.Min.Y, ParentGroup->ClippingRect.Min.Y),
			FMath::Min(BoundingRect.Max.X, ParentGroup->ClippingRect.Max.X),
			FMath::Min(BoundingRect.Max.Y, ParentGroup->ClippingRect.Max.Y));
		// 交集后整数尺寸≤0则跳过裁剪
		if (ClipRect.Min.X >= ClipRect.Max.X || ClipRect.Min.Y >= ClipRect.Max.Y)
			return;
		auto pClipGroup = FRenderGroup::Borrow();
		if (!pClipGroup)
			return;

		pClipGroup->bHasClipping = true;
		pClipGroup->ClippingRect = ClipRect;
		pClipGroup->WorldBounds = FBox2D(FVector2D(ClipRect.Min.X, ClipRect.Min.Y), // Min
			FVector2D(ClipRect.Max.X, ClipRect.Max.Y)								// Max
		);
		ParentGroup->AddChild(pClipGroup);
		CurrentGroup = pClipGroup;
	}

	// 先收集背景层：索引 [0, m_BackCount)
	for (int32 ChildIndex = 0; ChildIndex < m_BackCount; ++ChildIndex)
	{
		const TSharedRef<SDisplayObject>& CurChild = m_Children[ChildIndex];
		const SDisplayObject&			  DisplayObj = CurChild.Get();
		if (DisplayObj.IsVisible())
		{
			DisplayObj.CaculateGeometry(
				AllottedGeometry);
			DisplayObj.CollectRenderUnits(CurrentGroup, DisplayObj.GetPaintSpaceGeometry(),
				InAlpha * DisplayObj.GetRenderOpacity(), InSaturation * DisplayObj.GetSaturation());
		}
	}
	// 再收集本身
	for (auto& pSubMesh : SubMeshs)
	{
		auto Tex = FontCache->GetFontTexture(pSubMesh->TextureIndex);
		if (!Tex)
			continue;
		if (CurrentGroup->IsOutsize(pSubMesh->LocalBounds, LocalToWorld))
			continue;
		pSubMesh->RendererType = EFairyRendererType::Text;
		pSubMesh->FontTexture = Tex->GetSlateTexture();
		pSubMesh->LocalToWorld = LocalToWorld;
		pSubMesh->Alpha = InAlpha;
		pSubMesh->Saturation = InSaturation;
		pSubMesh->BlendMode = m_BlendMode;

		FRenderUnit* pUnit = FRenderUnit::FromSubMesh(StaticCastSharedPtr<FSubMesh>(pSubMesh));
		auto		 pTextSubMesh = StaticCastSharedPtr<FTextSubMesh>(pSubMesh);
		if (pTextSubMesh.IsValid() && pUnit)
		{
			pUnit->TextDrawType = pTextSubMesh->DrawType;
			pUnit->SdfEmOuterSpread = pTextSubMesh->SdfEmOuterSpread;
			pUnit->SdfPixelSpread = pTextSubMesh->SdfPixelSpread;
			pUnit->SdfBias = pTextSubMesh->SdfBias;
		}
		CurrentGroup->AddChild(pUnit);
	}
	// 最后收集前景层：索引 [m_BackCount, Num)
	for (int32 ChildIndex = m_BackCount; ChildIndex < m_Children.Num(); ++ChildIndex)
	{
		const TSharedRef<SDisplayObject>& CurChild = m_Children[ChildIndex];
		const SDisplayObject&			  DisplayObj = CurChild.Get();
		if (DisplayObj.IsVisible())
		{
			DisplayObj.CaculateGeometry(
				AllottedGeometry);
			DisplayObj.CollectRenderUnits(CurrentGroup, DisplayObj.GetPaintSpaceGeometry(),
				InAlpha * DisplayObj.GetRenderOpacity(), InSaturation * DisplayObj.GetSaturation());
		}
	}
	m_AttrChangeFlag = 0;
}

const SDisplayObject* SRichTextField::HitTest(const FVector2D& GlobalPoint) const
{

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