#include "Widgets/SSelectionShape.h"
#include "Widgets/NTexture.h"

IMPLEMENT_TYPE_ID(SSelectionShape)

SSelectionShape::SSelectionShape(UGObject* InGObject) : SDisplayObject(InGObject)
{
	m_pSubMesh = MakeShared<FSubMesh>();
	SetInteractable(false);
}

bool SSelectionShape::OnMouseButtonDown(const FPointerEvent& MouseEvent)
{
	if (OnClick.IsBound())
	{
		m_MousePressed = true;
		return true;
	}
	else
	{
		return false;
	}
}
bool SSelectionShape::OnMouseButtonUp(const FPointerEvent& MouseEvent)
{
	if (OnClick.IsBound() && m_MousePressed)
	{
		m_MousePressed = false;
		OnClick.Broadcast(MouseEvent);
		return true;
	}
	else
	{
		return false;
	}
}
void SSelectionShape::OnMouseEnter(const FPointerEvent& MouseEvent)
{
	if (OnRollover.IsBound())
		OnRollover.Broadcast(MouseEvent);
}
void SSelectionShape::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	if (OnRollout.IsBound())
		OnRollout.Broadcast(MouseEvent);
}

void SSelectionShape::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
	float InSaturation) const
{
	const_cast<SSelectionShape*>(this)->RebuildMesh(0);
	if (m_pSubMesh->Vertices.IsEmpty())
		return;

	m_pSubMesh->LocalToWorld = FMatrix44f(AllottedGeometry.GetAccumulatedRenderTransform().To3DMatrix());
	if (ParentGroup->IsOutsize(m_pSubMesh->LocalBounds, m_pSubMesh->LocalToWorld))
		return;

	m_pSubMesh->RendererType = EFairyRendererType::NoTexture;
	m_pSubMesh->Texture = nullptr;
	m_pSubMesh->Alpha = InAlpha;
	m_pSubMesh->Saturation = InSaturation;
	m_pSubMesh->BlendMode = m_BlendMode;

	ParentGroup->AddChild(FRenderUnit::FromSubMesh(m_pSubMesh));
	m_AttrChangeFlag = 0;
}
const SDisplayObject* SSelectionShape::HitTest(const FVector2D& GlobalPoint) const
{
	auto LocalPoint = m_CachedInverseDesktopTransform.TransformPoint(GlobalPoint);
	for (const FBox2D& Rect : m_Rects)
	{
		if (Rect.IsInside(LocalPoint))
		{
			return this;
		}
	}
	return nullptr;
}

void SSelectionShape::RebuildMesh(int32 LayerID)
{
	if (!m_bRebuildMesh)
		return;
	m_pSubMesh->Clear();
	if (m_Color.A > 0)
	{
		for (auto Rect : m_Rects)
		{
			m_pSubMesh->AddQuad(Rect, m_Color);
		}
		m_pSubMesh->AddTriangles();
	}
	m_bRebuildMesh = false;
}

TArray<TSharedPtr<SSelectionShape>> SSelectionShape::m_Pool;

TSharedPtr<SSelectionShape> SSelectionShape::Borrow()
{
	if (m_Pool.Num() > 0)
		return m_Pool.Pop();
	return MakeShared<SSelectionShape>(nullptr);
}
void SSelectionShape::Return(TSharedPtr<SSelectionShape> Value)
{
	m_Pool.Add(Value);
}
void SSelectionShape::Return(TArray<TSharedPtr<SSelectionShape>>& Values)
{
	m_Pool.Append(Values);
	Values.Empty();
}