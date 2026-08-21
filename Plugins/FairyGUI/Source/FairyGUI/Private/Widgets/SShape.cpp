#include "Widgets/SShape.h"
#include "Widgets/NTexture.h"
#include "Widgets/Mesh/RectMesh.h"
#include "Widgets/Mesh/RoundedRectMesh.h"
#include "Widgets/Mesh/EllipseMesh.h"
#include "Widgets/Mesh/PolygonMesh.h"
#include "Widgets/Mesh/RegularPolygonMesh.h"

IMPLEMENT_TYPE_ID(SShape)

SShape::SShape(UGObject* InGObject) : SDisplayObject(InGObject)
{
	m_pSubMesh = MakeShared<FSubMesh>();
}

void SShape::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
	float InSaturation) const
{
	const_cast<SShape*>(this)->RebuildMesh(0);

	if (m_pSubMesh->Vertices.IsEmpty())
		return;

	m_pSubMesh->LocalToWorld = FMatrix44f(AllottedGeometry.GetAccumulatedRenderTransform().To3DMatrix());
	if (ParentGroup->IsOutsize(m_pSubMesh->LocalBounds, m_pSubMesh->LocalToWorld))
		return;

	if (m_Texture && m_Texture->NativeTexture)
	{
		m_pSubMesh->RendererType = EFairyRendererType::Textured;
		m_pSubMesh->Texture = m_Texture->NativeTexture->GetResource();
	}
	else
	{
		m_pSubMesh->RendererType = EFairyRendererType::NoTexture;
		m_pSubMesh->Texture = nullptr;
	}

	m_pSubMesh->Alpha = InAlpha;
	m_pSubMesh->Saturation = InSaturation;
	m_pSubMesh->BlendMode = m_BlendMode;

	ParentGroup->AddChild(FRenderUnit::FromSubMesh(m_pSubMesh));
	m_AttrChangeFlag = 0;
}

const SDisplayObject* SShape::HitTest(const FVector2D& GlobalPoint) const
{
	FBox2D ContentRect(FVector2D::ZeroVector, m_Size);
	auto   LocalPoint = m_CachedInverseDesktopTransform.TransformPoint(GlobalPoint);
	bool   Result = false;
	switch (m_ShapeType)
	{
		case EFGUIShapeType::Rect:
			Result = ContentRect.IsInside(LocalPoint);
			break;
		case EFGUIShapeType::RoundRect:
			Result = FRoundedRectMesh::HitTest(
				ContentRect, LocalPoint, m_TopLeftRadius, m_TopRightRadius, m_BottomLeftRadius, m_BottomRightRadius);
			break;
		case EFGUIShapeType::Ellipse:
			Result = FEllipseMesh::HitTest(ContentRect, LocalPoint, m_LineWidth, m_StartDegree, m_EndDegree);
			break;
		case EFGUIShapeType::Polygon:
			Result = FPolygonMesh::HitTest(ContentRect, LocalPoint, m_PolygonPoints, m_bUsePercentPositions);
			break;
		case EFGUIShapeType::RegularPolygon:
			Result = FRegularPolygonMesh::HitTest(
				ContentRect, LocalPoint, m_PolygonSides, m_PolygonDistances, m_PolygonRotation);
			break;
	}
	if (Result)
		return this;
	return nullptr;
}

void SShape::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (m_Texture != nullptr)
		Collector.AddReferencedObject(m_Texture);
}

FString SShape::GetReferencerName() const
{
	return TEXT("SShape");
}

void SShape::RebuildMesh(int32 LayerID)
{
	if (!m_bRebuildMesh)
		return;
	m_bRebuildMesh = false;
	m_pSubMesh->Reset();
	if (m_ShapeType == EFGUIShapeType::None)
		return;

	FBox2D ContentRect(FVector2D::ZeroVector, m_Size);
	m_pSubMesh->ContentRect = ContentRect;
	m_pSubMesh->UVRect = m_Texture ? m_Texture->UVRect : FBox2D(ForceInit);
	m_pSubMesh->VertexColor = m_Color;

	switch (m_ShapeType)
	{
		case EFGUIShapeType::Rect:
			// Handle rectangle shape
			FRectMesh::BuildMesh(m_pSubMesh.Get(), m_LineWidth, m_LineColor, m_Colors.Num() > 0 ? &m_Colors : nullptr);
			break;
		case EFGUIShapeType::RoundRect:
			// Handle rounded rectangle shape
			FRoundedRectMesh::BuildMesh(m_pSubMesh.Get(), m_LineWidth, m_LineColor,
				m_CenterColor.IsSet() ? &m_CenterColor.GetValue() : nullptr, m_TopLeftRadius, m_TopRightRadius,
				m_BottomLeftRadius, m_BottomRightRadius);
			break;
		case EFGUIShapeType::Ellipse:
			// Handle ellipse shape
			FEllipseMesh::BuildMesh(m_pSubMesh.Get(), m_LineWidth, m_LineColor,
				m_CenterColor.IsSet() ? &m_CenterColor.GetValue() : nullptr, m_StartDegree, m_EndDegree);
			break;
		case EFGUIShapeType::Polygon:
			// Handle polygon shape
			FPolygonMesh::BuildMesh(m_pSubMesh.Get(), m_LineWidth, m_LineColor, m_PolygonPoints, m_bUsePercentPositions,
				m_Colors.Num() > 0 ? &m_Colors : nullptr, m_Texcoords.Num() > 0 ? &m_Texcoords : nullptr);
			break;
		case EFGUIShapeType::RegularPolygon:
			// Handle regular polygon shape
			FRegularPolygonMesh::BuildMesh(m_pSubMesh.Get(), m_LineWidth, m_LineColor, m_PolygonSides,
				m_PolygonDistances, m_PolygonRotation, m_CenterColor.IsSet() ? &m_CenterColor.GetValue() : nullptr);
			break;
		default:
			break;
	}

	if (m_Texture && m_Texture->bRotated)
	{
		float xMin = m_Texture->UVRect.Min.X;
		float yMin = m_Texture->UVRect.Min.Y;
		float xMax = m_Texture->UVRect.Max.X;
		for (FSlateVertex& Vertex : m_pSubMesh->Vertices)
		{
			auto& vec = Vertex.TexCoords;
			float tmp = vec[1];
			vec[1] = yMin + xMax - vec[0];
			vec[0] = xMin + tmp - yMin;
			Vertex.MaterialTexCoords[0] = vec[0];
			Vertex.MaterialTexCoords[1] = vec[1];
		}
	}
}
