#include "Widgets/SFImage.h"
#include "Widgets/NTexture.h"
#include "UI/FieldTypes.h"

IMPLEMENT_TYPE_ID(SFImage)

SFImage::SFImage(UGObject* InGObject)
    : SDisplayObject(InGObject)
    , m_TextureScale(1, 1)
{
	m_pSubMesh = MakeShared<FSubMesh>();
	m_Texture = UNTexture::GetWhiteTexture();
}

void SFImage::SetTexture(UNTexture* InTexture)
{
	if (m_Texture != InTexture)
	{
		m_Texture = InTexture;

		if (InTexture != nullptr && m_Size.IsZero())
		{
			SetSize(InTexture->GetSize());
		}
		if (m_Texture == nullptr)
			m_Texture = UNTexture::GetWhiteTexture();
		m_bRebuildMesh = true;
	}
}

void SFImage::SetNativeSize()
{
	if (m_Texture != nullptr && m_Texture != UNTexture::GetWhiteTexture())
	{
		SetSize(m_Texture->GetSize());
	}
}

void SFImage::SetScale9Grid(const TOptional<FBox2D>& InGridRect)
{
	m_Scale9Grid = InGridRect;
}

void SFImage::SetScaleByTile(bool bInScaleByTile)
{
	if (m_bScaleByTile != bInScaleByTile)
	{
		m_bScaleByTile = bInScaleByTile;
		m_bRebuildMesh = true;
	}
}

void SFImage::SetTileGridIndice(int32 InTileGridIndex)
{
	if (m_TileGridIndice != InTileGridIndex)
	{
		m_TileGridIndice = InTileGridIndex;
		m_bRebuildMesh = true;
	}
}

EFillMethod SFImage::GetFillMethod() const
{
	return m_FillMesh.IsValid() ? m_FillMesh->Method : EFillMethod::None;
}

void SFImage::SetFillMethod(EFillMethod InMethod)
{
	if (!m_FillMesh.IsValid())
	{
		if (InMethod == EFillMethod::None)
			return;

		m_FillMesh = MakeUnique<FFillMesh>();
	}

	if (m_FillMesh->Method != InMethod)
	{
		m_FillMesh->Method = InMethod;
		m_bRebuildMesh = true;
	}
}

int32 SFImage::GetFillOrigin() const
{
	return m_FillMesh.IsValid() ? m_FillMesh->Origin : 0;
}

void SFImage::SetFillOrigin(int32 InOrigin)
{
	if (!m_FillMesh.IsValid())
		m_FillMesh = MakeUnique<FFillMesh>();

	if (m_FillMesh->Origin != InOrigin)
	{
		m_FillMesh->Origin = InOrigin;
		m_bRebuildMesh = true;
	}
}

bool SFImage::IsFillClockwise() const
{
	return m_FillMesh.IsValid() ? m_FillMesh->bClockwise : true;
}

void SFImage::SetFillClockwise(bool bInClockwise)
{
	if (!m_FillMesh.IsValid())
		m_FillMesh = MakeUnique<FFillMesh>();

	if (m_FillMesh->bClockwise != bInClockwise)
	{
		m_FillMesh->bClockwise = bInClockwise;
		m_bRebuildMesh = true;
	}
}

float SFImage::GetFillAmount() const
{
	return m_FillMesh.IsValid() ? m_FillMesh->Amount : 0;
}

void SFImage::SetFillAmount(float InAmount)
{
	if (!m_FillMesh.IsValid())
		m_FillMesh = MakeUnique<FFillMesh>();

	if (m_FillMesh->Amount != InAmount)
	{
		m_FillMesh->Amount = InAmount;
		m_bRebuildMesh = true;
	}
}

void SFImage::SetFlip(EFlipType Value)
{
	if (m_Flip != Value)
	{
		m_Flip = Value;
		m_bRebuildMesh = true;
	}
}

void SFImage::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
	float InSaturation) const
{
	const_cast<SFImage*>(this)->RebuildMesh(0);
	if (m_Texture == nullptr || m_Texture->NativeTexture == nullptr)
		return;
	if (m_pSubMesh->Vertices.IsEmpty())
		return;

	m_pSubMesh->LocalToWorld = FMatrix44f(AllottedGeometry.GetAccumulatedRenderTransform().To3DMatrix());
	if (ParentGroup->IsOutsize(m_pSubMesh->LocalBounds, m_pSubMesh->LocalToWorld))
		return;

	m_pSubMesh->RendererType = EFairyRendererType::Textured;
	m_pSubMesh->Texture = m_Texture->NativeTexture->GetResource();
	m_pSubMesh->Alpha = InAlpha;
	m_pSubMesh->Saturation = InSaturation;
	m_pSubMesh->BlendMode = m_BlendMode;

	ParentGroup->AddChild(FRenderUnit::FromSubMesh(m_pSubMesh));
	m_AttrChangeFlag = 0;
}

void SFImage::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (m_Texture != nullptr)
		Collector.AddReferencedObject(m_Texture);
}

FString SFImage::GetReferencerName() const
{
	return TEXT("SFImage");
}

void SFImage::RebuildMesh(int32 LayerID)
{
	if (!m_bRebuildMesh)
		return;
	m_bRebuildMesh = false;
	m_pSubMesh->Reset();
	if (m_Texture == nullptr || m_Texture->NativeTexture == nullptr)
		return;

	FBox2D ContentRect(FVector2D::ZeroVector, m_Size);
	m_pSubMesh->ContentRect = ContentRect;
	FBox2D UVRect = m_Texture->UVRect;
	if (m_Flip != EFlipType::None)
	{
		if (m_Flip == EFlipType::Horizontal || m_Flip == EFlipType::Both)
		{
			float tmp = UVRect.Min.X;
			UVRect.Min.X = UVRect.Max.X;
			UVRect.Max.X = tmp;
		}
		if (m_Flip == EFlipType::Vertical || m_Flip == EFlipType::Both)
		{
			float tmp = UVRect.Min.Y;
			UVRect.Min.Y = UVRect.Max.Y;
			UVRect.Max.Y = tmp;
		}
	}
	m_pSubMesh->UVRect = UVRect;
	m_pSubMesh->VertexColor = m_Color;

	if (m_FillMesh.IsValid() && m_FillMesh->Method != EFillMethod::None)
	{
		m_FillMesh->FillMesh(m_pSubMesh.Get());
	}
	else if (m_bScaleByTile)
	{
		if (m_Texture->Root == m_Texture && m_Texture->NativeTexture->GetTextureAddressX() == TextureAddress::TA_Mirror
			&& m_Texture->NativeTexture->GetTextureAddressY() == TextureAddress::TA_Mirror)
		{
			UVRect.Max = UVRect.Min + UVRect.GetSize() * ContentRect.GetSize() / m_Texture->GetSize() * m_TextureScale;
			m_pSubMesh->AddQuad(ContentRect, m_Color, UVRect);
			m_pSubMesh->AddTriangles();
		}
		else
		{
			ContentRect.Max = ContentRect.Min + ContentRect.GetSize() * m_TextureScale;
			TileFill(m_pSubMesh.Get(), ContentRect, UVRect, m_Texture->GetSize());
			m_pSubMesh->AddTriangles();
		}
	}
	else if (m_Scale9Grid.IsSet())
	{
		SliceFill(m_pSubMesh.Get());
	}
	else
	{
		FBox2D rect = m_Texture->GetDrawRect(ContentRect);
		m_pSubMesh->AddQuad(rect, m_Color, UVRect);
		m_pSubMesh->AddTriangles();
	}

	if (m_Texture->bRotated)
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

void SFImage::SliceFill(FSubMesh* pSubMesh)
{
	const SlateIndex TRIANGLES_9_GRID[] = { 4, 0, 1, 1, 5, 4, 5, 1, 2, 2, 6, 5, 6, 2, 3, 3, 7, 6, 8, 4, 5, 5, 9, 8, 9,
		5, 6, 6, 10, 9, 10, 6, 7, 7, 11, 10, 12, 8, 9, 9, 13, 12, 13, 9, 10, 10, 14, 13, 14, 10, 11, 11, 15, 14 };

	const int32 gridTileIndice[] = { -1, 0, -1, 2, 4, 3, -1, 1, -1 };

	static float gridX[4];
	static float gridY[4];
	static float gridTexX[4];
	static float gridTexY[4];

	FBox2D GridRect = m_Scale9Grid.GetValue();
	FBox2D ContentRect = m_pSubMesh->ContentRect;
	ContentRect.Max = ContentRect.Min + ContentRect.GetSize() * m_TextureScale;
	FBox2D	  UVRect = m_pSubMesh->UVRect;
	FVector2D TextureSize = m_Texture->GetSize();

	if (m_Flip != EFlipType::None)
	{
		if (m_Flip == EFlipType::Horizontal || m_Flip == EFlipType::Both)
		{
			GridRect.Min.X = TextureSize.X - GridRect.Max.X;
			GridRect.Max.X = GridRect.Min.X + GridRect.GetSize().X;
		}

		if (m_Flip == EFlipType::Vertical || m_Flip == EFlipType::Both)
		{
			GridRect.Min.Y = TextureSize.Y - GridRect.Max.Y;
			GridRect.Max.Y = GridRect.Min.Y + GridRect.GetSize().Y;
		}
	}

	FVector2D Scale = UVRect.GetSize() / TextureSize;

	gridTexX[0] = UVRect.Min.X;
	gridTexX[1] = UVRect.Min.X + GridRect.Min.X * Scale.X;
	gridTexX[2] = UVRect.Min.X + GridRect.Max.X * Scale.X;
	gridTexX[3] = UVRect.Max.X;
	gridTexY[0] = UVRect.Min.Y;
	gridTexY[1] = UVRect.Min.Y + GridRect.Min.Y * Scale.Y;
	gridTexY[2] = UVRect.Min.Y + GridRect.Max.Y * Scale.Y;
	gridTexY[3] = UVRect.Max.Y;

	if (ContentRect.GetSize().X >= (TextureSize.X - GridRect.GetSize().X))
	{
		gridX[1] = GridRect.Min.X;
		gridX[2] = ContentRect.GetSize().X - (TextureSize.X - GridRect.Max.X);
		gridX[3] = ContentRect.GetSize().X;
	}
	else
	{
		float tmp = GridRect.Min.X / (TextureSize.X - GridRect.Max.X);
		tmp = ContentRect.GetSize().X * tmp / (1 + tmp);
		gridX[1] = tmp;
		gridX[2] = tmp;
		gridX[3] = ContentRect.GetSize().X;
	}

	if (ContentRect.GetSize().Y >= (TextureSize.Y - GridRect.GetSize().Y))
	{
		gridY[1] = GridRect.Min.Y;
		gridY[2] = ContentRect.GetSize().Y - (TextureSize.Y - GridRect.Max.Y);
		gridY[3] = ContentRect.GetSize().Y;
	}
	else
	{
		float tmp = GridRect.Min.Y / (TextureSize.Y - GridRect.Max.Y);
		tmp = ContentRect.GetSize().Y * tmp / (1 + tmp);
		gridY[1] = tmp;
		gridY[2] = tmp;
		gridY[3] = ContentRect.GetSize().Y;
	}

	if (m_TileGridIndice == 0)
	{
		for (int32 cy = 0; cy < 4; cy++)
		{
			for (int32 cx = 0; cx < 4; cx++)
				pSubMesh->AddVertex(FVector2D(gridX[cx] / m_TextureScale.X, gridY[cy] / m_TextureScale.Y), m_Color,
					FVector2D(gridTexX[cx], gridTexY[cy]));
		}
		pSubMesh->AddTriangles(TRIANGLES_9_GRID, sizeof(TRIANGLES_9_GRID) / sizeof(SlateIndex));
	}
	else
	{
		FBox2D drawRect;
		FBox2D texRect;
		int32  row, col;
		int32  part;

		for (int32 pii = 0; pii < 9; pii++)
		{
			col = pii % 3;
			row = pii / 3;
			part = gridTileIndice[pii];
			drawRect = FBox2D(FVector2D(gridX[col], gridY[row]), FVector2D(gridX[col + 1], gridY[row + 1]));
			texRect = FBox2D(FVector2D(gridTexX[col], gridTexY[row]), FVector2D(gridTexX[col + 1], gridTexY[row + 1]));

			if (part != -1 && (m_TileGridIndice & (1 << part)) != 0)
			{
				TileFill(pSubMesh, drawRect, texRect,
					(part == 0 || part == 1 || part == 4) ? GridRect.GetSize() : drawRect.GetSize());
			}
			else
			{
				drawRect.Min /= m_TextureScale.X;
				drawRect.Max = drawRect.Min + drawRect.GetSize() * m_TextureScale;

				pSubMesh->AddQuad(drawRect, m_Color, texRect);
			}
		}

		pSubMesh->AddTriangles();
	}
}

void SFImage::TileFill(
	FSubMesh* pSubMesh, const FBox2D& ContentRect, const FBox2D& UVRect, const FVector2D& TextureSize)
{
	FVector2D cnt = ContentRect.GetSize() / TextureSize;
	cnt.Set(FMath::CeilToInt(cnt.X), FMath::CeilToInt(cnt.Y));
	FVector2D tailSize = ContentRect.GetSize() - (cnt - FVector2D(1, 1)) * TextureSize;
	for (int32 i = 0; i < cnt.X; i++)
	{
		for (int32 j = 0; j < cnt.Y; j++)
		{
			FBox2D UVTmp = UVRect;
			if (i == cnt.X - 1)
				UVTmp.Max.X = FMath::Lerp(UVRect.Min.X, UVRect.Max.X, tailSize.X / TextureSize.X);
			if (j == cnt.Y - 1)
				UVTmp.Max.Y = FMath::Lerp(UVRect.Min.Y, UVRect.Max.Y, tailSize.Y / TextureSize.Y);

			FVector2D Min = ContentRect.Min + FVector2D(i, j) * TextureSize;
			FBox2D	  drawRect = FBox2D(Min,
				   Min
					   + FVector2D(
						   i == (cnt.X - 1) ? tailSize.X : TextureSize.X, j == (cnt.Y - 1) ? tailSize.Y : TextureSize.Y));

			drawRect.Min /= m_TextureScale;
			drawRect.Max = drawRect.Min + drawRect.GetSize() / m_TextureScale;

			pSubMesh->AddQuad(drawRect, m_Color, UVTmp);
		}
	}
}
