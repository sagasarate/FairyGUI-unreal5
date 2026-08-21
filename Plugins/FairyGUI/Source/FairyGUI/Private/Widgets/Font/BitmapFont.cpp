#include "Widgets/Font/BitmapFont.h"
#include "Widgets/NTexture.h"
#include "UI/UIConfig.h"
#include "FairyCommons.h"

// ============================================================================
// FBMPFontGlyphSequence 实现
// ============================================================================

IMPLEMENT_TYPE_ID(FBMPFontGlyphSequence)

int32 FBMPFontGlyphSequence::GetGlyphCount()
{
	return m_Glyphs.Num();
}

float FBMPFontGlyphSequence::GetBaseline(float Scale)
{
	// 基线 = 从传入 DrawGlyph 的 Y 坐标顶部到基线的距离
	// LineHeight 是原始字体大小中的值，需同时乘以 m_Scale 和 Scale
	if (m_Glyphs.Num() > 0)
	{
		for (int32 i = 0; i < m_Glyphs.Num(); i++)
		{
			if (m_Glyphs[i] != nullptr)
				return m_Glyphs[i]->LineHeight * m_Scale * Scale;
		}
	}
	// 默认基线 = 字体大小 * 缩放
	return m_TextFormat.Size * Scale;
}

bool FBMPFontGlyphSequence::GetGlyph(int32 GlyphIndex, float& GlyphWidth, float& GlyphHeight, int32& CharIndex)
{
	if (GlyphIndex < 0 || GlyphIndex >= m_Glyphs.Num())
		return false;

	FBMPFontGlyph* pGlyph = m_Glyphs[GlyphIndex];
	CharIndex = m_CharIndices[GlyphIndex];

	if (pGlyph == nullptr)
	{
		// 空格或缺失字符 —— 返回空格宽度
		GlyphWidth = m_TextFormat.Size * m_Scale * 0.5f;
		GlyphHeight = m_TextFormat.Size * m_Scale;
		return true;
	}

	GlyphWidth = pGlyph->XAdvance * m_Scale;
	GlyphHeight = pGlyph->LineHeight * m_Scale;
	return true;
}

bool FBMPFontGlyphSequence::DrawGlyph(
	int32 Start, int32 Count, FTextMesh& TextMesh, float X, float Y, float LetterSpacing, float Scale)
{
	if (!m_ParentFont.IsValid())
	{
		UE_LOG(LogFairyGUI, Error, TEXT("FBMPFontGlyphSequence does not have parent font!"));
		return false;
	}

	auto Font = Cast<UBitmapFont>(m_ParentFont.Get());
	if (!Font)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("FBMPFontGlyphSequence's parent font is not UBitmapFont!"));
		return false;
	}

	if (Start < 0 || Start + Count > m_Glyphs.Num())
	{
		UE_LOG(LogFairyGUI, Error, TEXT("FBMPFontGlyphSequence: invalid glyph index range!"));
		return false;
	}

	const float	  EffectiveScale = m_Scale * Scale;
	const bool	  bCanTint = Font->CanTint();
	const FColor& TextColor = m_TextFormat.Color;
	FVector2D	  ShadowOffset = m_TextFormat.ShadowOffset * Scale;

	// 获取位图字体的纹理资源
	UNTexture* NTex = Font->GetTexture();
	if (!NTex || !NTex->NativeTexture)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("UBitmapFont::DrawGlyph: font texture is null!"));
		return false;
	}
	FTextureResource* TexResource = NTex->NativeTexture->GetResource();
	if (!TexResource)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("UBitmapFont::DrawGlyph: failed to get texture resource!"));
		return false;
	}

	int32 End = Start + Count;
	float StartX = X;

	for (int32 i = Start; i < End; i++)
	{
		FBMPFontGlyph* pGlyph = m_Glyphs[i];

		if (pGlyph == nullptr)
		{
			// 空格或缺失字符 —— 仅推进 X 坐标
			StartX += m_TextFormat.Size * EffectiveScale * 0.5f + LetterSpacing;
			continue;
		}

		// 计算屏幕空间绘制矩形 (UE Y-down 坐标)
		// Offset.Y = 从行顶部到字形顶部的距离
		float  DrawX = StartX + pGlyph->Offset.X * EffectiveScale;
		float  DrawY = Y + pGlyph->Offset.Y * EffectiveScale;
		float  DrawW = pGlyph->Size.X * EffectiveScale;
		float  DrawH = pGlyph->Size.Y * EffectiveScale;
		FBox2D VertRect(FVector2D(DrawX, DrawY), FVector2D(DrawX + DrawW, DrawY + DrawH));

		// UV 矩形 —— 直接使用字形中预烘焙的 UV 数据
		FBox2D UVRect = pGlyph->UVRect;

		// 确定染色颜色
		FColor DrawColor = bCanTint ? TextColor : FColor::White;

		// 绘制阴影
		if (!ShadowOffset.IsNearlyZero())
		{
			FBox2D ShadowRect(FVector2D(DrawX + ShadowOffset.X, DrawY + ShadowOffset.Y),
				FVector2D(DrawX + DrawW + ShadowOffset.X, DrawY + DrawH + ShadowOffset.Y));

			auto pShadowSubMesh =
				TextMesh.GetSubMesh(ETextSubMeshType::Shadow, TexResource, ETextDrawType::AlphaBitmap);
			if (!pShadowSubMesh)
				return false;
			pShadowSubMesh->AddQuad(ShadowRect, m_TextFormat.ShadowColor, UVRect);
			pShadowSubMesh->AddTriangles(-4);
		}

		// 绘制描边：向 N 个方向偏移绘制，用描边颜色
		if (m_TextFormat.OutlineSize > 0)
		{
			float OutlineDist = m_TextFormat.OutlineSize * Scale;
			if (OutlineDist > 0)
			{
				auto pOutlineSubMesh =
					TextMesh.GetSubMesh(ETextSubMeshType::Outline, TexResource, ETextDrawType::AlphaBitmap);
				if (!pOutlineSubMesh)
					return false;

				const bool bEightDir = FUIConfig::Config.BitmapFontOutlineType == EBitmapFontOutlineType::EightDir;

				auto DrawOutlinePass = [&](float OffX, float OffY) {
					FBox2D OutlineRect(
						FVector2D(DrawX + OffX, DrawY + OffY), FVector2D(DrawX + DrawW + OffX, DrawY + DrawH + OffY));
					pOutlineSubMesh->AddQuad(OutlineRect, m_TextFormat.OutlineColor, UVRect);
				};

				// 四方向：上下左右
				DrawOutlinePass(-OutlineDist, 0);
				DrawOutlinePass(OutlineDist, 0);
				DrawOutlinePass(0, -OutlineDist);
				DrawOutlinePass(0, OutlineDist);

				// 八方向：额外加四个对角线
				if (bEightDir)
				{
					DrawOutlinePass(-OutlineDist, -OutlineDist);
					DrawOutlinePass(OutlineDist, -OutlineDist);
					DrawOutlinePass(-OutlineDist, OutlineDist);
					DrawOutlinePass(OutlineDist, OutlineDist);
				}

				pOutlineSubMesh->AddTriangles(-4 * (bEightDir ? 8 : 4));
			}
		}

		// 绘制主体字形
		auto pTextSubMesh = TextMesh.GetSubMesh(ETextSubMeshType::Text, TexResource, ETextDrawType::ColorBitmap);
		if (!pTextSubMesh)
			return false;
		pTextSubMesh->AddQuad(VertRect, DrawColor, UVRect);
		pTextSubMesh->AddTriangles(-4);

		// 推进 X 坐标
		StartX += pGlyph->XAdvance * EffectiveScale + LetterSpacing;
	}

	// TODO: 下划线 / 删除线 —— 需要独立的 Line 纹理，待后续集成 STextField::CollectRenderUnits 时完善

	return true;
}

void FBMPFontGlyphSequence::GetCharToGlyphMap(TArray<int32>& Map)
{
	for (int32 i = 0; i < m_Glyphs.Num(); i++)
	{
		int32 CharIndex = m_CharIndices[i];
		if (CharIndex < Map.Num())
			Map[CharIndex] = i;
	}
}

void FBMPFontGlyphSequence::Clear()
{
	m_Glyphs.Reset();
	m_CharIndices.Reset();
	m_Scale = 1.0f;
	m_ParentFont.Reset();
	m_PoolID = 0;
}

// ============================================================================
// UBitmapFont 实现
// ============================================================================

UBitmapFont::UBitmapFont()
{
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		m_Size = 0;
		m_bResizable = true;
		m_bHasChannel = false;
		m_bCanTint = true;
		m_Scale = 1.0f;
	}
}

void UBitmapFont::AddChar(TCHAR ch, const FBMPFontGlyph& Glyph)
{
	m_GlyphDict.Add((int32)ch, Glyph);
}

void UBitmapFont::RemoveChar(TCHAR ch)
{
	m_GlyphDict.Remove((int32)ch);
}

void UBitmapFont::ClearChars()
{
	m_GlyphDict.Empty();
}

void UBitmapFont::SetFormat(const FNTextFormat& Format)
{
	m_TextFormat = Format;

	// 根据是否可缩放计算 Scale
	if (m_bResizable && m_Size > 0)
		m_Scale = (float)Format.Size / m_Size;
	else
		m_Scale = 1.0f;
}

TSharedPtr<FBaseGlyphSequence> UBitmapFont::GetGlyph(const FString& Text)
{
	TSharedPtr<FBMPFontGlyphSequence> pSeq = nullptr;

	if (m_GlyphPool.Num() > 0)
		pSeq = m_GlyphPool.Pop();
	else
		pSeq = MakeShared<FBMPFontGlyphSequence>();
	if (!pSeq)
		return nullptr;

	pSeq->SetParent(this);
	pSeq->SetTextFormat(m_TextFormat);
	pSeq->SetScale(m_Scale);

	// 遍历文本中的每个字符，查找对应的位图字形
	TArray<FBMPFontGlyph*> Glyphs;
	TArray<int32>		   CharIndices;
	Glyphs.Reserve(Text.Len());
	CharIndices.Reserve(Text.Len());

	for (int32 i = 0; i < Text.Len(); i++)
	{
		TCHAR		   ch = Text[i];
		FBMPFontGlyph* pGlyph = m_GlyphDict.Find((int32)ch);
		Glyphs.Add(pGlyph); // 可能为 nullptr (缺失字符)
		CharIndices.Add(i);
	}

	pSeq->SetGlyphs(Glyphs, CharIndices);
	return pSeq;
}

bool UBitmapFont::HasCharacter(TCHAR ch)
{
	return ch == ' ' || m_GlyphDict.Contains((int32)ch);
}

int UBitmapFont::GetLineHeight(int Size)
{
	if (m_GlyphDict.Num() > 0)
	{
		// 取字典中第一个字形的 LineHeight 作为代表
		for (auto& Pair : m_GlyphDict)
		{
			if (m_bResizable && m_Size > 0)
				return FMath::RoundToInt((float)Pair.Value.LineHeight * Size / m_Size);
			else
				return Pair.Value.LineHeight;
		}
	}
	// 默认行高
	return Size;
}

bool UBitmapFont::ReturnGlyph(TSharedPtr<FBaseGlyphSequence> pGlyphSequence)
{
	if (pGlyphSequence->IsA<FBMPFontGlyphSequence>())
	{
		pGlyphSequence->Clear();
		m_GlyphPool.Add(StaticCastSharedPtr<FBMPFontGlyphSequence>(pGlyphSequence));
		return true;
	}
	return false;
}
