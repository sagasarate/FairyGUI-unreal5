#include "Widgets/Font/DynamicFont.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Engine/Font.h"
#include "Fonts/FontCache.h"
#include "Fonts/SlateFontInfo.h"
#include "UI/UIConfig.h"
#include "ImageUtils.h"
#include "FairyCommons.h"
#include "Widgets/Mesh/TextSubMesh.h"

const float UDynamicFont::SUP_SCALE = 0.58f;
const float UDynamicFont::SUP_OFFSET = 0.33f;
const float UDynamicFont::LINE_HEIGHT_FACTOR = 1.25f;

IMPLEMENT_TYPE_ID(FDynamicFontGlyphSequence)

int32 FDynamicFontGlyphSequence::GetGlyphCount()
{
	if (m_ShapedSequence)
		return m_ShapedSequence->GetGlyphsToRender().Num();
	return 0;
}
float FDynamicFontGlyphSequence::GetBaseline(float Scale)
{
	// 返回从传入DrawGlyph的Y坐标到文本基线的距离
	// 推导: 基线 = DrawY + BearingY = Y + (YOffset + BaseLine + 行高) * Scale
	// 因此从Y到基线的距离 = GetTextBaseline() + 行高 (近似YOffset≈0时)
	float TextBaseLine = m_ShapedSequence->GetTextBaseline();
	float Baseline = (TextBaseLine + m_TextFormat.Size * UDynamicFont::LINE_HEIGHT_FACTOR) * Scale;
	return Baseline;
}
bool FDynamicFontGlyphSequence::GetGlyph(int32 GlyphIndex, float& GlyphWidth, float& GlyphHeight, int32& CharIndex)
{
	const TArray<FShapedGlyphEntry>& Glyphs = m_ShapedSequence->GetGlyphsToRender();
	if (GlyphIndex < 0 || GlyphIndex >= Glyphs.Num())
		return false;
	auto& Glyph = Glyphs[GlyphIndex];
	if (m_TextFormat.SpecialStyle != SpecialStyle::None)
		GlyphWidth = Glyph.XAdvance * UDynamicFont::SUP_SCALE;
	else
		GlyphWidth = Glyph.XAdvance;
	GlyphHeight = m_TextFormat.Size * UDynamicFont::LINE_HEIGHT_FACTOR;
	CharIndex = Glyph.SourceIndex;
	return true;
}
bool FDynamicFontGlyphSequence::DrawGlyph(
	int32 Start, int32 Count, FTextMesh& TextMesh, float X, float Y, float LetterSpacing, float Scale)
{
	if (!m_ParentFont.IsValid())
	{
		UE_LOG(LogFairyGUI, Error, TEXT("FDynamicFontGlyphSequence not have parent font!"));
		return false;
	}
	auto Font = Cast<UDynamicFont>(m_ParentFont.Get());
	if (!Font)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("FDynamicFontGlyphSequence's parent font is not UDynamicFont!"));
		return false;
	}
	TSharedPtr<FSlateFontCache> FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
	if (!FontCache.IsValid())
	{
		UE_LOG(LogFairyGUI, Error, TEXT("failed to get font cache!"));
		return false;
	}
	const TArray<FShapedGlyphEntry>& Glyphs = m_ShapedSequence->GetGlyphsToRender();
	if (Start < 0 || Start + Count > Glyphs.Num())
	{
		UE_LOG(LogFairyGUI, Error, TEXT("invalid glyph index!"));
		return false;
	}
	if (Count < 0)
		return true;
	if (m_TextFormat.SpecialStyle == SpecialStyle::Subscript)
		Y = Y
			+ FMath::RoundToInt(
				m_TextFormat.Size * UDynamicFont::LINE_HEIGHT_FACTOR * Scale * (1 - UDynamicFont::SUP_SCALE));
	else if (m_TextFormat.SpecialStyle == SpecialStyle::Superscript)
		Y = Y - FMath::RoundToInt(m_TextFormat.Size * Scale * UDynamicFont::SUP_SCALE * UDynamicFont::SUP_OFFSET);
	if (m_TextFormat.SpecialStyle != SpecialStyle::None)
		Scale *= UDynamicFont::SUP_SCALE;
	bool  bBold = m_TextFormat.bBold && ((Font->GetFontTypeFaceSet() & EFontTypeFace::Bold) == 0);
	float Skew = 0;
	if (m_TextFormat.bItalic && ((Font->GetFontTypeFaceSet() & EFontTypeFace::Italic) == 0))
		Skew = UBaseFont::DEFAULT_ITALIC_ANGLE;
	int32 End = Start + Count;

	float StartX = X;
	auto  ShadowOffset = m_TextFormat.ShadowOffset * Scale;
	float OutlineSize = m_TextFormat.OutlineSize * Scale;

	// Probe first glyph for SDF support to choose outline/bold strategy
	bool  bSdfSupported = false;
	float SdfEmOuterSpread = 0.0f;

	int32 FirstChar = Start;

	FFontOutlineSettings ProbeSettings;
	ProbeSettings.OutlineSize = 0;
	while (FirstChar < Glyphs.Num() && (!Glyphs[FirstChar].bIsVisible)) // 跳过不可见字符
		FirstChar++;
	if (FirstChar >= End) // 所有字符不可见，直接返回
		return true;
	auto ProbeSdfData = FontCache->GetSdfGlyphFontAtlasData(Glyphs[FirstChar], ProbeSettings);
	if (ProbeSdfData.Valid && ProbeSdfData.bSupportsSdf)
	{
		bSdfSupported = true;
		SdfEmOuterSpread = ProbeSdfData.EmOuterSpread;
		// 用探针字形计算逐字体一致的 SdfPixelSpread 参考值
		// 同字号所有字符共用同一个值，消除 AtlasSlot 逐字形差异导致的 AA 不一致
		if (ProbeSdfData.USize > 0)
		{
			auto* pProbeTex = FontCache->GetFontTexture(ProbeSdfData.TextureIndex);
			if (pProbeTex)
			{
				auto* pProbeRes = pProbeTex->GetSlateTexture();
				if (pProbeRes)
				{
					float TextureWidth = (float)pProbeRes->GetWidth();

					// 计算当前字形在 Atlas 槽位中的总 Spread (单位: Texels)
					// 总 Spread(em) = EmOuterSpread + EmInnerSpread
					// 转换为 Texels = TotalEmSpread * FontPointSize (或者通过 USize / Em 比例转换)
					float TotalEmSpread = ProbeSdfData.EmOuterSpread + ProbeSdfData.EmInnerSpread;

					// 如果能获取到固定的 Spread Texels (例如默认 8.0f)，直接使用；
					// 否则利用 EmSpread 与 USize 计算：
					float SpreadInTexels = 8.0f; // 💡 绝大多数 UE/Slate SDF 配置下为 4.0 或 8.0
					if (TotalEmSpread > 0.0f && ProbeSdfData.Metrics.Width > 0)
					{
						// 用 Metrics 宽度推算 Spread 占据的 Texel 数量
						float EmWidth = ProbeSdfData.Metrics.Width / FMath::Max(m_TextFormat.Size, 1.0f);
						SpreadInTexels = ProbeSdfData.USize * (TotalEmSpread / (EmWidth + TotalEmSpread));
					}

					// 保证 SpreadInTexels 有合理最小值
					SpreadInTexels = FMath::Max(SpreadInTexels, 1.0f);

					// SdfPixelSpread = 纹理总宽度 / 单个 Spread 的 Texel 数量
					// 这样在 Shader 中: fwidth(UV) * SdfPixelSpread 就能精准算出屏幕 1px 对应的 Dist 步长
					m_RefSdfPixelSpread = TextureWidth / SpreadInTexels;
				}
			}
		}
	}

	// SDF mode parameter precomputation
	float SdfBiasBase = 0.5f;
	float BoldSdfOffset = 0.0f;	   // 粗体：填充阈值向外偏移量（约 1 像素）
	float OutlineSdfOffset = 0.0f; // 描边：描边环宽度，相对于填充阈值向外偏移
	if (bSdfSupported)
	{
		if (FUIConfig::Config.OutlineSizeIsSDFOffset)
		{
			OutlineSdfOffset = m_TextFormat.OutlineSize * 1.5f;
		}
		else
		{ // OutlineSize 是逻辑像素，转为 em 需用 m_TextFormat.Size
			float OutlineEm = m_TextFormat.OutlineSize / FMath::Max(m_TextFormat.Size, 1.0f);
			// SDF spread 映射到屏幕像素：将 em 单位描边宽度转为 SDF 阈值偏移
			if (SdfEmOuterSpread > 0.0f)
				OutlineSdfOffset = 0.5f * OutlineEm / SdfEmOuterSpread;
		}
		// Bold: sqrt 平滑缩放，小字减细避免过粗，大字保持约 1px
		float BoldEm = 0.06f / FMath::Sqrt(FMath::Max(m_TextFormat.Size, 1.0f));
		if (SdfEmOuterSpread > 0.0f)
			BoldSdfOffset = 0.5f * BoldEm / SdfEmOuterSpread;
	}
	int32		  LineTexIndex = 0;
	ETextDrawType DrawType = ETextDrawType::AlphaBitmap;
	for (int32 i = Start; i < End; i++)
	{
		auto& Glyph = Glyphs[i];

		float TextSdfBias = SdfBiasBase;
		if (bBold && bSdfSupported)
			TextSdfBias = FMath::Max(0.1f, SdfBiasBase - BoldSdfOffset);

		if (!ShadowOffset.IsNearlyZero())
		{
			if (bSdfSupported)
			{
				if (!DrawGlyph(FontCache, Glyph, TextMesh, StartX + ShadowOffset.X, Y + ShadowOffset.Y, Scale, Skew, 0,
						m_TextFormat.ShadowColor, ETextSubMeshType::Shadow, DrawType, LineTexIndex, TextSdfBias))
					return false;
			}
			else
			{
				if (!DrawGlyph(FontCache, Glyph, TextMesh, StartX + ShadowOffset.X, Y + ShadowOffset.Y, Scale, Skew,
						bBold ? 1 : 0, m_TextFormat.ShadowColor, ETextSubMeshType::Shadow, DrawType, LineTexIndex))
					return false;
			}
		}

		if (bSdfSupported)
		{
			// 单 pass 描边：描边参数随 Text SubMesh 传递，shader 内完成混合
			FColor outlineColor = FColor::Transparent;
			float  outlineBias = TextSdfBias;
			if (m_TextFormat.OutlineSize > 0)
			{
				outlineColor = m_TextFormat.OutlineColor;
				outlineBias = FMath::Max(0.05f, TextSdfBias - OutlineSdfOffset);
			}
			if (!DrawGlyph(FontCache, Glyph, TextMesh, StartX, Y, Scale, Skew, 0, m_TextFormat.Color,
					ETextSubMeshType::Text, DrawType, LineTexIndex, TextSdfBias, outlineColor, outlineBias))
				return false;
		}
		else
		{
			// Bitmap path: original logic
			if (!ShadowOffset.IsNearlyZero())
			{
				if (!DrawGlyph(FontCache, Glyph, TextMesh, StartX + ShadowOffset.X, Y + ShadowOffset.Y, Scale, Skew,
						bBold ? 1 : 0, m_TextFormat.ShadowColor, ETextSubMeshType::Shadow, DrawType, LineTexIndex))
					return false;
			}
			if (m_TextFormat.OutlineSize > 0 && m_OutlineShapedSequence.IsValid())
			{
				auto& OutlineGlyph = m_OutlineShapedSequence->GetGlyphsToRender()[i];
				if (!DrawGlyph(FontCache, OutlineGlyph, TextMesh, StartX, Y, Scale, Skew,
						m_TextFormat.OutlineSize + (bBold ? 1 : 0), m_TextFormat.OutlineColor,
						ETextSubMeshType::Outline, DrawType, LineTexIndex))
					return false;
			}
			if (!DrawGlyph(FontCache, Glyph, TextMesh, StartX, Y, Scale, Skew, bBold ? 1 : 0, m_TextFormat.Color,
					ETextSubMeshType::Text, DrawType, LineTexIndex))
				return false;
		}

		StartX += Glyph.XAdvance * Scale + LetterSpacing;
	}
	if (m_TextFormat.bUnderline || m_TextFormat.bStrikethrough)
	{
		float  LineWidth = 0;
		float  LineHeight = FMath::Max(m_TextFormat.Size * Scale / 16.0f, 1.0f);
		FBox2D LineUVRect;
		for (int32 i = Start; i < End; i++)
		{
			auto& Glyph = Glyphs[i];
			LineWidth += Glyph.XAdvance * Scale + LetterSpacing;
		}
		if (LineTexIndex >= 0)
		{
			if (DrawType == ETextDrawType::AlphaBitmap)
				DrawType = ETextDrawType::Line;
			else
				DrawType = ETextDrawType::SDFLine;
			if (m_TextFormat.bUnderline)
			{
				float LineY = Y + m_TextFormat.Size * UDynamicFont::LINE_HEIGHT_FACTOR * Scale - LineHeight;
				if (!ShadowOffset.IsNearlyZero())
				{
					FBox2D DrawRect(FVector2D(X + ShadowOffset.X, LineY + ShadowOffset.Y),
						FVector2D(X + ShadowOffset.X + LineWidth, LineY + ShadowOffset.Y + LineHeight));
					if (!DrawLine(TextMesh, DrawRect, LineTexIndex, LineUVRect, m_TextFormat.ShadowColor,
							ETextSubMeshType::Shadow, DrawType))
						return false;
				}
				if (m_TextFormat.OutlineSize > 0 && m_OutlineShapedSequence.IsValid())
				{
					FBox2D DrawRect(FVector2D(X - OutlineSize, LineY - OutlineSize),
						FVector2D(X + LineWidth + OutlineSize, LineY + LineHeight + OutlineSize));
					if (!DrawLine(TextMesh, DrawRect, LineTexIndex, LineUVRect, m_TextFormat.OutlineColor,
							ETextSubMeshType::Outline, DrawType))
						return false;
				}
				FBox2D DrawRect(FVector2D(X, LineY), FVector2D(X + LineWidth, LineY + LineHeight));
				if (!DrawLine(TextMesh, DrawRect, LineTexIndex, LineUVRect, m_TextFormat.Color, ETextSubMeshType::Text,
						DrawType))
					return false;
			}
			if (m_TextFormat.bStrikethrough)
			{
				float LineY = Y + ((m_TextFormat.Size * UDynamicFont::LINE_HEIGHT_FACTOR - LineHeight) / 2.0f) * Scale;
				if (!ShadowOffset.IsNearlyZero())
				{
					FBox2D DrawRect(FVector2D(X + ShadowOffset.X, LineY + ShadowOffset.Y),
						FVector2D(X + ShadowOffset.X + LineWidth, LineY + ShadowOffset.Y + LineHeight));
					if (!DrawLine(TextMesh, DrawRect, LineTexIndex, LineUVRect, m_TextFormat.ShadowColor,
							ETextSubMeshType::Shadow, DrawType))
						return false;
				}
				if (m_TextFormat.OutlineSize > 0 && m_OutlineShapedSequence.IsValid())
				{
					FBox2D DrawRect(FVector2D(X - OutlineSize, LineY - OutlineSize),
						FVector2D(X + LineWidth + OutlineSize, LineY + LineHeight + OutlineSize));
					if (!DrawLine(TextMesh, DrawRect, LineTexIndex, LineUVRect, m_TextFormat.OutlineColor,
							ETextSubMeshType::Outline, DrawType))
						return false;
				}
				FBox2D DrawRect(FVector2D(X, LineY), FVector2D(X + LineWidth, LineY + LineHeight));
				if (!DrawLine(TextMesh, DrawRect, LineTexIndex, LineUVRect, m_TextFormat.Color, ETextSubMeshType::Text,
						DrawType))
					return false;
			}
		}
	}

	return true;
}
void FDynamicFontGlyphSequence::GetCharToGlyphMap(TArray<int32>& Map)
{
	const TArray<FShapedGlyphEntry>& Glyphs = m_ShapedSequence->GetGlyphsToRender();
	for (int32 i = 0; i < Glyphs.Num(); i++)
	{
		auto& Glyph = Glyphs[i];
		if (Glyph.SourceIndex < Map.Num())
			Map[Glyph.SourceIndex] = i;
	}
}
void FDynamicFontGlyphSequence::Clear()
{
	m_ShapedSequence.Reset();
	m_OutlineShapedSequence.Reset();
	m_ParentFont.Reset();
	// m_Text.Empty();
	m_RefSdfPixelSpread = 0.0f;
	m_PoolID = 0;
}

bool FDynamicFontGlyphSequence::DrawGlyph(TSharedPtr<FSlateFontCache> FontCache, const FShapedGlyphEntry& Glyph,
	FTextMesh& TextMesh, float X, float Y, float Scale, float Skew, int32 OutlineSize, const FColor& Color,
	ETextSubMeshType Type, ETextDrawType& DrawType, int32& TextureIndex, float SdfBias, const FColor& InOutlineColor,
	float InOutlineBias)
{
	if (!Glyph.bIsVisible) // 跳过不可见字符
		return true;

	FFontOutlineSettings OutlineSettings;
	OutlineSettings.OutlineSize = OutlineSize;

	DrawType = ETextDrawType::AlphaBitmap;
	float SdfEmOuterSpread = 0.0f;
	float SdfEmInnerSpread = 0.0f;

	// 图集数据（用局部变量统一存储）
	// 参考 UE ElementBatcher.cpp：SDF 的屏幕四边形用 Metrics（视觉尺寸不含 spread），UV 用完整槽位
	int16 AtlasStartU = 0, AtlasStartV = 0, AtlasUSize = 0, AtlasVSize = 0;
	float QuadMeshOffsetX = 0, QuadMeshOffsetY = 0;
	float QuadMeshWidth = 0, QuadMeshHeight = 0;
	TextureIndex = 0;
	bool bAtlasValid = false;

	// 始终优先尝试 SDF 路径，引擎内部根据 FontFace 设置 + CVar 自动判断
	auto SdfAtlasData = FontCache->GetSdfGlyphFontAtlasData(Glyph, OutlineSettings);
	if (SdfAtlasData.Valid && SdfAtlasData.bSupportsSdf)
	{
		AtlasStartU = SdfAtlasData.StartU;
		AtlasStartV = SdfAtlasData.StartV;
		AtlasUSize = SdfAtlasData.USize;
		AtlasVSize = SdfAtlasData.VSize;
		// SDF: 屏幕四边形使用 Metrics（字形视觉边界框，不含 spread）
		QuadMeshOffsetX = SdfAtlasData.Metrics.BearingX;
		QuadMeshOffsetY = SdfAtlasData.Metrics.BearingY;
		QuadMeshWidth = SdfAtlasData.Metrics.Width;
		QuadMeshHeight = SdfAtlasData.Metrics.Height;
		TextureIndex = SdfAtlasData.TextureIndex;
		bAtlasValid = true;
		auto* pSdfTexture = FontCache->GetFontTexture(TextureIndex);
		DrawType = (pSdfTexture && pSdfTexture->GetContentType() == ESlateFontAtlasContentType::Msdf)
			? ETextDrawType::MSDF
			: ETextDrawType::SDF;
		SdfEmOuterSpread = SdfAtlasData.EmOuterSpread;
		SdfEmInnerSpread = SdfAtlasData.EmInnerSpread;
		// 诊断日志：打印每个字形的Spread和Atlas槽宽，用于分析AA一致性
		// UE_LOG(LogFairyGUI, Log,
		// 	TEXT("[SDF Glyph] Char=%c | EmOuter=%.4f EmInner=%.4f TotalSpread=%.4f | AtlasSlot=%dx%d | "
		// 		 "Metrics=%.0fx%.0f Bearing=%.0f,%.0f"),
		// 	Glyph.SourceIndex < m_Text.Len() ? m_Text[Glyph.SourceIndex] : TEXT('?'), SdfEmOuterSpread, SdfEmInnerSpread,
		// 	SdfEmOuterSpread + SdfEmInnerSpread, AtlasUSize, AtlasVSize, QuadMeshWidth, QuadMeshHeight,
		// 	QuadMeshOffsetX, QuadMeshOffsetY);
	}

	if (!bAtlasValid)
	{
		// 回退到 Bitmap 路径
		DrawType = ETextDrawType::AlphaBitmap;
		auto AtlasData = FontCache->GetShapedGlyphFontAtlasData(Glyph, OutlineSettings);
		if (!AtlasData.Valid)
		{
			UE_LOG(LogFairyGUI, Error, TEXT("failed to get font atlas data!"));
			return false;
		}
		AtlasStartU = AtlasData.StartU;
		AtlasStartV = AtlasData.StartV;
		AtlasUSize = AtlasData.USize;
		AtlasVSize = AtlasData.VSize;
		// Bitmap: 屏幕四边形使用槽位偏移和尺寸
		QuadMeshOffsetX = AtlasData.HorizontalOffset;
		QuadMeshOffsetY = AtlasData.VerticalOffset;
		QuadMeshWidth = AtlasData.USize;
		QuadMeshHeight = AtlasData.VSize;
		TextureIndex = AtlasData.TextureIndex;
		bAtlasValid = true;
	}

	auto pSubMesh = TextMesh.GetSubMesh(Type, TextureIndex, DrawType);
	if (!pSubMesh)
		return false;

	auto Texture = FontCache->GetFontTexture(TextureIndex);
	if (!Texture)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("failed to get font texture!"));
		return false;
	}
	auto pRes = Texture->GetSlateTexture();
	if (!pRes)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("font texture is invalid!"));
		return false;
	}

	float		InvTextureSizeX = 1.0f / pRes->GetWidth();
	float		InvTextureSizeY = 1.0f / pRes->GetHeight();
	const float BitmapRenderScale = Glyph.GetBitmapRenderScale();

	float U, V, SizeU_UV, SizeV_UV;
	if (DrawType == ETextDrawType::SDF || DrawType == ETextDrawType::MSDF || DrawType == ETextDrawType::SDFLine)
	{
		// SDF: 完整槽位UV——不加HalfTexel偏移，不减1像素
		// SDF依赖纹理过滤的连续梯度，手动偏移会破坏距离场采样精度
		U = AtlasStartU * InvTextureSizeX;
		V = AtlasStartV * InvTextureSizeY;
		SizeU_UV = AtlasUSize * InvTextureSizeX;
		SizeV_UV = AtlasVSize * InvTextureSizeY;
	}
	else
	{
		// Bitmap: 半像素偏移使双线性采样落在图集像素中心，避免边缘模糊
		const float HalfTexelX = 0.5f * InvTextureSizeX;
		const float HalfTexelY = 0.5f * InvTextureSizeY;
		U = AtlasStartU * InvTextureSizeX + HalfTexelX;
		V = AtlasStartV * InvTextureSizeY + HalfTexelY;
		SizeU_UV = (AtlasUSize - 1.0f) * InvTextureSizeX;
		SizeV_UV = (AtlasVSize - 1.0f) * InvTextureSizeY;
	}

	const float BaseLine = m_ShapedSequence->GetTextBaseline();

	// BitmapRenderScale 统一应用于视觉度量（Bearing + Size），与UE ElementBatcher保持一致
	const float SizeX = QuadMeshWidth * BitmapRenderScale;
	const float SizeY = QuadMeshHeight * BitmapRenderScale;
	const float DrawX = X + (QuadMeshOffsetX * BitmapRenderScale + Glyph.XOffset) * Scale;
	const float DrawY = Y - QuadMeshOffsetY * BitmapRenderScale * Scale
		+ ((Glyph.YOffset + BaseLine + m_TextFormat.Size * UDynamicFont::LINE_HEIGHT_FACTOR) * Scale);
	FBox2D DrawRect(FVector2D(DrawX, DrawY), FVector2D(DrawX + SizeX * Scale, DrawY + SizeY * Scale));
	FBox2D UVRect(FVector2D(U, V), FVector2D(U + SizeU_UV, V + SizeV_UV));
	if (FMath::IsNearlyZero(Skew))
		Skew = 0;
	else
		Skew = SizeY * FMath::Tan(FMath::DegreesToRadians(Skew));
	pSubMesh->AddQuad(DrawRect, Color, UVRect, Skew);
	pSubMesh->AddTriangles(-4);
	pSubMesh->SdfEmOuterSpread = SdfEmOuterSpread;
	pSubMesh->SdfEmInnerSpread = SdfEmInnerSpread;
	// 使用探针字形的参考值（逐字体一致），避免 AtlasSlot 逐字形差异导致 AA 宽度不一致
	pSubMesh->SdfPixelSpread = m_RefSdfPixelSpread;
	pSubMesh->SdfBias = SdfBias;
	pSubMesh->OutlineColor = InOutlineColor;
	pSubMesh->OutlineBias = InOutlineBias;
	return true;
}

bool FDynamicFontGlyphSequence::DrawLine(FTextMesh& TextMesh, const FBox2D& DrawRect, int32 TextureIndex,
	const FBox2D& UVRect, const FColor& Color, ETextSubMeshType Type, ETextDrawType DrawType)
{
	auto pSubMesh = TextMesh.GetSubMesh(Type, TextureIndex, DrawType);
	if (pSubMesh)
	{
		pSubMesh->AddQuad(DrawRect, Color, UVRect);
		pSubMesh->AddTriangles(-4);
		return true;
	}
	return false;
}

UDynamicFont::UDynamicFont()
{
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		m_Name = G_DEFAULT_FONT_NAME;
		SetFont(FUIConfig::Config.DefaultFont.LoadSynchronous(), m_Name);
	}
}

bool UDynamicFont::SetFont(UFont* Font, FName FontName)
{
	if (!IsValid(Font))
		return false;

	m_FontInfo.FontObject = Font;
	m_FontTypeFace = EFontTypeFace::None;
	auto CompositeFont = m_FontInfo.GetCompositeFont();
	if (CompositeFont)
	{
		for (const FTypefaceEntry& Entry : CompositeFont->DefaultTypeface.Fonts)
		{
			FString StyleName = Entry.Name.ToString().ToLower();

			if (StyleName.Contains(TEXT("bold")))
			{
				m_FontTypeFace = EFontTypeFace::Bold;
			}

			if (StyleName.Contains(TEXT("italic")))
			{
				m_FontTypeFace |= EFontTypeFace::Italic;
			}
		}
	}
	if (FontName.IsNone())
		m_Name = Font->GetFName();
	else
		m_Name = FontName;

	return true;
}

bool UDynamicFont::LoadFont(const FString& FontAssetPath, FName FontName)
{
	UFont* Font = LoadObject<UFont>(this, FontAssetPath);
	return SetFont(Font, FontName);
}

void UDynamicFont::SetFormat(const FNTextFormat& Format)
{
	m_TextFormat = Format;
	float PointSize = m_TextFormat.Size * 0.75f;
	m_FontInfo.Size = PointSize;
	if (m_TextFormat.bBold && (m_FontTypeFace & EFontTypeFace::Bold) != 0)
	{
		m_FontInfo.TypefaceFontName = TEXT("bold");
		m_FontTypeFaceSet |= EFontTypeFace::Bold;
	}
	else if (m_TextFormat.bItalic && (m_FontTypeFace & EFontTypeFace::Italic) != 0)
	{
		m_FontInfo.TypefaceFontName = TEXT("italic");
		m_FontTypeFaceSet |= EFontTypeFace::Italic;
	}
}
TSharedPtr<FBaseGlyphSequence> UDynamicFont::GetGlyph(const FString& Text)
{
	TSharedPtr<FDynamicFontGlyphSequence> pGlypSequence = nullptr;
	if (m_GlyphPool.Num() > 0)
		pGlypSequence = m_GlyphPool.Pop();
	else
		pGlypSequence = MakeShared<FDynamicFontGlyphSequence>();
	if (!pGlypSequence)
		return nullptr;
	TSharedPtr<FSlateFontCache> FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
	if (!FontCache.IsValid())
		return nullptr;

	FShapedGlyphSequenceRef ShapedSequence = FontCache->ShapeBidirectionalText(
		Text, m_FontInfo, 1.0f, TextBiDi::ETextDirection::LeftToRight, ETextShapingMethod::Auto);

	pGlypSequence->SetParent(this);
	pGlypSequence->SetShapedSequence(ShapedSequence);
	pGlypSequence->SetTextFormat(m_TextFormat);
	// pGlypSequence->SetText(Text);

	if (m_TextFormat.OutlineSize)
	{
		m_FontInfo.OutlineSettings.OutlineSize = m_TextFormat.OutlineSize;
		FShapedGlyphSequenceRef OutlineShapedSequence = FontCache->ShapeBidirectionalText(
			Text, m_FontInfo, 1.0f, TextBiDi::ETextDirection::LeftToRight, ETextShapingMethod::Auto);
		pGlypSequence->SetOutlineShapedSequence(OutlineShapedSequence);
		m_FontInfo.OutlineSettings.OutlineSize = 0;
	}

	// const TArray<FShapedGlyphEntry>& Glyphs = ShapedSequence->GetGlyphsToRender();
	// auto AtlasData = FontCache->GetSdfGlyphFontAtlasData(Glyphs[0], m_FontInfo.OutlineSettings);
	// UDynamicFont::SaveFontAtlasToFile(AtlasData.TextureIndex, FPaths::ProjectSavedDir() / "Font.png");

	return pGlypSequence;
}
bool UDynamicFont::HasCharacter(TCHAR ch)
{
	return true;
}
int UDynamicFont::GetLineHeight(int size)
{
	return FMath::RoundToInt(size * LINE_HEIGHT_FACTOR);
}
bool UDynamicFont::ReturnGlyph(TSharedPtr<FBaseGlyphSequence> pGlyphSequence)
{
	if (pGlyphSequence->IsA<FDynamicFontGlyphSequence>())
	{
		pGlyphSequence->Clear();
		m_GlyphPool.Add(StaticCastSharedPtr<FDynamicFontGlyphSequence>(pGlyphSequence));
		return true;
	}
	return false;
}

void UDynamicFont::SaveFontAtlasToFile(int32 TextureIndex, const FString& FilePath)
{
	auto FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();

	ISlateFontTexture* FontTex = FontCache->GetFontTexture(TextureIndex);
	if (!FontTex)
		return;

	// 1. 获取像素数据 (CPU 端副本)
	TArray<uint8> RawData;
	FontTex->GetAtlasDataCopy(RawData);

	if (RawData.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Font texture %d: GetAtlasDataCopy returned empty"), TextureIndex);
		return;
	}

	// 2. 获取尺寸和格式
	FSlateShaderResource* TexResource = FontTex->GetSlateTexture();
	const int32			  Width = (int32)TexResource->GetWidth();
	const int32			  Height = (int32)TexResource->GetHeight();

	ESlateFontAtlasContentType ContentType = FontTex->GetContentType();
	if (ContentType == ESlateFontAtlasContentType::Alpha)
	{
		// ★ Alpha 图集：单通道灰度 → 需要扩展为 RGBA
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		for (int32 i = 0; i < RawData.Num(); i++)
		{
			uint8 Alpha = RawData[i];
			Pixels[i] = FColor(Alpha, Alpha, Alpha, Alpha); // 灰度图，Alpha 置 255
		}

		// 保存为 PNG
		FIntPoint		DestSize(Width, Height);
		TArray64<uint8> CompressedBitmap;
		FImageUtils::PNGCompressImageArray(DestSize.X, DestSize.Y, Pixels, CompressedBitmap);
		FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath);
	}
	else // Color 或 Msdf (4 字节/像素，BGRA 排列)
	{
		// 直接构造 FColor 数组
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Width * Height);
		const uint8* RawPtr = RawData.GetData();
		for (int32 i = 0; i < Width * Height; i++)
		{
			Pixels[i] = FColor(RawPtr[2], RawPtr[1], RawPtr[0], RawPtr[3]); // B→R, G→G, R→B, A→A
			RawPtr += 4;
		}

		TArray64<uint8> CompressedBitmap;
		FImageUtils::PNGCompressImageArray(Width, Height, Pixels, CompressedBitmap);
		FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath);
	}

	UE_LOG(LogTemp, Log, TEXT("Font atlas saved: %s (%dx%d, %d bytes)"), *FilePath, Width, Height, RawData.Num());
}