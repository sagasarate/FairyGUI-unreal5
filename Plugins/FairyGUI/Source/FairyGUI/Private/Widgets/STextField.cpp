#include "Widgets/STextField.h"
#include "Widgets/SInputTextField.h"
#include "Widgets/SRichTextField.h"
#include "Utils/HTML/HTMLParser.h"
#include "Utils/HTML/HTMLLINK.h"
#include "Utils/UBBParser.h"
#include "Widgets/Font/FontManager.h"
#include "UI/GObject.h"
#include "UI/UIPackage.h"
#include "UI/UIConfig.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Utils/HTML/HTMLObject.h"
#include "Internationalization/BreakIterator.h"

CIDStorage<STextField::LineCharInfo> STextField::LineCharInfo::Pool;

const int32 STextField::GUTTER_X = 2;
const int32 STextField::GUTTER_Y = 2;
const float STextField::HTML_OBJ_BASELINE = 0.8f;
const int32 STextField::ELLIPSIS_LENGTH = 2;

STextField::LineCharInfo* STextField::LineCharInfo::Borrow()
{
	if (Pool.GetBufferSize() == 0)
		Pool.Create(256, 256, FUIConfig::Config.TextLineInfoPoolGrowLimit);
	LineCharInfo* pInfo = Pool.NewObject();
	if (pInfo == nullptr)
	{
		UE_LOG(
			LogFairyGUI, Error, TEXT("LineCharInfo pool is full(%u/%u)!"), Pool.GetObjectCount(), Pool.GetBufferSize());
	}
	return pInfo;
}

void STextField::LineCharInfo::Return(LineCharInfo* pValue)
{
	uint32 ID = pValue->PoolID;
	pValue->Clear();
	Pool.DeleteObject(ID);
}

void STextField::LineCharInfo::Return(TArray<LineCharInfo*>& Values)
{
	for (auto& Info : Values)
		Return(Info);
	Values.Empty();
}

CIDStorage<STextField::LineBlock> STextField::LineBlock::Pool;

void STextField::LineBlock::Clear()
{
	PoolID = 0;
	pElement = nullptr;
	Baseline = 0;
	Width = 0;
	Height = 0;
	X = 0;
	Y = 0;
	CharIndex = 0;
	pGlyphSequence = nullptr;
	LineCharInfo::Return(Chars);
}

STextField::LineBlock* STextField::LineBlock::Borrow()
{
	if (Pool.GetBufferSize() == 0)
		Pool.Create(256, 256, FUIConfig::Config.TextLineInfoPoolGrowLimit);
	LineBlock* pInfo = Pool.NewObject();
	if (pInfo == nullptr)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("LineBlock pool is full(%u/%u)!"), Pool.GetObjectCount(), Pool.GetBufferSize());
	}
	return pInfo;
}
void STextField::LineBlock::Return(LineBlock* pValue)
{
	uint32 ID = pValue->PoolID;
	pValue->Clear();
	Pool.DeleteObject(ID);
}
void STextField::LineBlock::Return(TArray<LineBlock*>& Values)
{
	for (auto& Info : Values)
		Return(Info);
	Values.Empty();
}

CIDStorage<STextField::LineInfo> STextField::LineInfo::Pool;

void STextField::LineInfo::Clear()
{
	PoolID = 0;
	Baseline = 0;
	Width = 0;
	Height = 0;
	X = 0;
	Y = 0;
	CharIndex = 0;
	CharCount = 0;
	LineBlock::Return(Blocks);
}

STextField::LineInfo* STextField::LineInfo::Borrow()
{
	if (Pool.GetBufferSize() == 0)
		Pool.Create(256, 256, FUIConfig::Config.TextLineInfoPoolGrowLimit);
	LineInfo* pInfo = Pool.NewObject();
	if (pInfo == nullptr)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("LineInfo pool is full(%u/%u)!"), Pool.GetObjectCount(), Pool.GetBufferSize());
	}
	return pInfo;
}

void STextField::LineInfo::Return(LineInfo* pValue)
{
	uint32 ID = pValue->PoolID;
	pValue->Clear();
	Pool.DeleteObject(ID);
}

void STextField::LineInfo::Return(TArray<LineInfo*>& Values)
{
	for (auto& Info : Values)
		Return(Info);
	Values.Empty();
}

FString STextField::ELLIPSIS_TEXT(TEXT("..."));

IMPLEMENT_TYPE_ID(STextField)

STextField::STextField(UGObject* InGObject) : SDisplayObject(InGObject)
{
	SetInteractable(false);
}
STextField::~STextField()
{
	ClearAll();
}

void STextField::SetText(const FString& InText)
{
	if (InText.Compare(m_Text, ESearchCase::CaseSensitive) == 0)
		return;

	m_Text = InText;
	m_bRebuildText = true;
}

void STextField::SetAutoSize(EAutoSizeType InAutoSize)
{
	if (m_AutoSize != InAutoSize)
	{
		m_AutoSize = InAutoSize;
		m_bRebuildGlyph = true;
	}
}

void STextField::SetAlign(EAlignType Align)
{
	if (m_TextFormat.Align != Align)
	{
		m_TextFormat.Align = Align;
		for (auto pElement : m_HTMLElements)
			pElement->GetFormat() = m_TextFormat;
		m_bRebuildMesh = true;
	}
}

void STextField::SetVerticalAlign(EVerticalAlignType Align)
{
	if (m_VerticalAlign != Align)
	{
		m_VerticalAlign = Align;
		m_bRebuildMesh = true;
	}
}

void STextField::SetSingleLine(bool bInSingleLine)
{
	if (m_bSingleLine != bInSingleLine)
	{
		m_bSingleLine = bInSingleLine;
		m_bRebuildGlyph = true;
	}
}

void STextField::SetMaxWidth(float InMaxWidth)
{
	if (m_MaxWidth != InMaxWidth)
	{
		m_MaxWidth = InMaxWidth;
		m_bRebuildGlyph = true;
	}
}

FVector2D STextField::GetTextSize()
{
	BuildLines();
	return FVector2D(m_TextWidth, m_TextHeight);
}

void STextField::EnableWrapBreakByWord(bool beEnable)
{
	if (m_bWrapBreakByWord != beEnable)
	{
		m_bWrapBreakByWord = beEnable;
		m_bRebuildGlyph = true;
	}
}
void STextField::SetTextClipType(ETextClipType InClipType)
{
	if (m_ClipType != InClipType)
	{
		m_ClipType = InClipType;
		if (m_ClipType == ETextClipType::ByPixel)
			SetClipToBounds(true);
		else
			SetClipToBounds(false);
		m_bRebuildMesh = true;
	}
}
void STextField::SetUBBEnabled(bool bEnabled)
{
	if (m_bUBBEnabled != bEnabled)
	{
		m_bUBBEnabled = bEnabled;
		m_bRebuildText = true;
	}
}
void STextField::GetLinesShape(int32 StartCharIndex, int32 EndCharIndex, bool bClipped, TArray<FBox2D>& ResultRects)
{
	float LBound = GUTTER_X;
	float TBound = GUTTER_Y;
	float RBound = GetWidth() - GUTTER_X;
	float BBound = GetHeight() - GUTTER_Y;
	float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;
	for (auto pLine : m_Lines)
	{
		if (StartCharIndex >= pLine->CharIndex + pLine->CharCount)
			continue;
		if (EndCharIndex <= pLine->CharIndex)
			break;
		float Y = pLine->Y;
		float H = pLine->Height;
		for (auto pBlock : pLine->Blocks)
		{
			int32 BlockStart = pBlock->CharIndex;
			int32 BlockEnd = pBlock->CharIndex + pBlock->Chars.Num();

			// 无交集，跳过
			if (EndCharIndex <= BlockStart || StartCharIndex >= BlockEnd)
				continue;

			int32 FirstCharIdx = FMath::Max(StartCharIndex, BlockStart);
			int32 LastCharIdx = FMath::Min(EndCharIndex, BlockEnd);

			float X = pBlock->X;
			float W = 0;

			if (pBlock->Chars.Num() > 0)
			{
				for (auto pChar : pBlock->Chars)
				{
					if (pChar->CharIndex < FirstCharIdx)
						X += pChar->Width + LetterSpacing;
					else if (pChar->CharIndex < LastCharIdx)
						W += pChar->Width + LetterSpacing;
					else
						break;
				}
			}
			else
			{
				// 无字符数据时（如图片Block），使用Block整体尺寸
				W = pBlock->Width;
			}

			if (W > 0)
			{
				if (bClipped)
				{
					if (X + W < LBound || X > RBound || Y + H < TBound || Y > BBound)
						continue;
					X = FMath::Max(X, LBound);
					Y = FMath::Max(Y, TBound);
					W = FMath::Min(X + W, RBound) - X;
					H = FMath::Min(Y + H, BBound) - Y;
				}
				ResultRects.Add(FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
			}
		}
	}
}

void STextField::SetTextFormat(const FNTextFormat& InFormat)
{
	if (&InFormat != &m_TextFormat)
	{
		m_TextFormat = InFormat;
		for (auto pElement : m_HTMLElements)
			pElement->GetFormat() = m_TextFormat;
		m_bRebuildText = true;
	}
}

void STextField::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
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

	const_cast<STextField*>(this)->BuildLines();

	TArray<TSharedPtr<FTextSubMesh>> SubMeshs;
	m_TextMesh.GetSubMeshs(SubMeshs);

	FMatrix44f LocalToWorld = FMatrix44f(AllottedGeometry.GetAccumulatedRenderTransform().To3DMatrix());

	FRenderGroup* CurrentGroup = ParentGroup;
	if (m_ClipType == ETextClipType::ByPixel)
	{
		// 与父组裁剪范围取交集，反映祖先累积的真实裁剪约束
		FIntRect ClipRect = FIntRect(FMath::Max(BoundingRect.Min.X, ParentGroup->ClippingRect.Min.X),
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

	for (auto& pSubMesh : SubMeshs)
	{
		if (CurrentGroup->IsOutsize(pSubMesh->LocalBounds, LocalToWorld))
			continue;
		pSubMesh->RendererType = EFairyRendererType::Text;
		if (pSubMesh->TextureIndex >= 0)
		{
			// 动态字体路径 —— 从 Slate 字体缓存获取纹理
			auto Tex = FontCache->GetFontTexture(pSubMesh->TextureIndex);
			if (!Tex)
				continue;
			pSubMesh->FontTexture = Tex->GetSlateTexture();
		}
		else if (pSubMesh->Texture == nullptr)
		{
			UE_LOG(LogFairyGUI, Error, TEXT("Text submesh has no texture!"));
			continue;
		}

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
	m_AttrChangeFlag = 0;
}

void STextField::ClearGlyphs()
{
	LineInfo::Return(m_Lines);
	m_Lines.Empty();
	for (auto pGlyphSequence : m_GlyphSequences)
	{
		pGlyphSequence->Release();
	}
	m_GlyphSequences.Empty();
}
void STextField::ClearAll()
{
	ClearGlyphs();
	FHTMLElement::Return(m_HTMLElements);
	m_HTMLElements.Empty();
}

void STextField::BuildLines()
{
	if (!m_IsFontAtlasReleasedRegistered)
	{
		TSharedPtr<FSlateFontCache> FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
		if (FontCache)
		{
			FontCache->OnReleaseResources().AddSP(this, &STextField::OnFontAtlasReleased);
		}
		m_IsFontAtlasReleasedRegistered = true;
	}
	if (m_bRebuildText)
	{
		ParseText(m_Text, m_bUBBEnabled, IsA<SRichTextField>());
		m_bRebuildText = false;
		m_bRebuildGlyph = true;
	}

	if (m_bRebuildGlyph)
	{
		m_FontSizeScale = 1.0f;
		BuildGlyphs();
		if (m_AutoSize == EAutoSizeType::Shrink)
			DoShrink();
		else if (m_AutoSize == EAutoSizeType::Ellipsis)
			DoEllipsis();
		UpdateSize();
		m_bRebuildGlyph = false;
		m_bRebuildMesh = true;
	}
	if (m_bRebuildMesh)
	{
		BuildMesh();
		UpdateHTMLObjects();
		m_bRebuildMesh = false;
	}
}

void STextField::ParseText(const FString& Text, bool bEnableUBBParser, bool bEnableHTMLParser)
{
	FHTMLElement::Return(m_HTMLElements);
	m_HTMLElements.Empty();
	if (Text.Len() == 0)
		return;

	TSharedPtr<SRichTextField> pRich = nullptr;
	if (IsA<SRichTextField>())
		pRich = StaticCastSharedRef<SRichTextField>(AsShared());
	if (m_bUBBEnabled)
	{
		FHTMLParser::DefaultParser.Parse(
			FUBBParser::DefaultParser.Parse(Text), m_TextFormat, m_HTMLElements, FHTMLParser::DefaultParseOptions);
	}
	else if (bEnableHTMLParser)
	{
		FHTMLParser::DefaultParser.Parse(Text, m_TextFormat, m_HTMLElements, FHTMLParser::DefaultParseOptions);
	}
	else
	{
		auto pNewElement = FHTMLElement::Borrow();
		pNewElement->SetType(EHTMLElementType::Text);
		pNewElement->GetFormat() = m_TextFormat;
		pNewElement->GetText() = Text;
		pNewElement->SetCharIndex(0);
		m_HTMLElements.Add(pNewElement);
	}

	for (int32 i = m_HTMLElements.Num() - 1; i >= 0; i--)
	{
		FHTMLElement* pElement = m_HTMLElements[i];
		if (pElement->GetType() == EHTMLElementType::Text)
		{
			FString& SrcText = pElement->GetText();
			for (int32 j = SrcText.Len() - 1; j >= 0; j--)
			{
				if (IsHighSurrogate(SrcText[j]))
				{
					if (j < SrcText.Len() - 1 && IsLowSurrogate(SrcText[j + 1]))
					{
						if (pRich)
						{
							uint32 EmojiKey =
								((uint32)SrcText[j + 1] & 0x03FF) + ((((uint32)SrcText[j] & 0x03FF) + 0x40) << 10);
							int32 LeftLen = SrcText.Len() - j - 2;
							int32 InsertIndex = i + 1;
							if (LeftLen > 0)
							{
								auto pNewElement = FHTMLElement::Borrow();
								pNewElement->SetType(pElement->GetType());
								pNewElement->GetFormat() = pElement->GetFormat();
								pNewElement->GetText() = SrcText.Right(LeftLen);
								pNewElement->SetCharIndex(pElement->GetCharIndex() + j + 2);
								m_HTMLElements.Insert(pNewElement, InsertIndex);
							}
							SRichTextField::Emoji* pEmoji = pRich->GetEmojis().Find(EmojiKey);
							if (pEmoji)
							{
								auto pNewElement = FHTMLElement::Borrow();
								pNewElement->SetType(EHTMLElementType::Image);
								pNewElement->GetFormat() = pElement->GetFormat();
								pNewElement->GetAttributes().Set(TEXT("src"), pEmoji->Url);
								if (pEmoji->Width != 0)
									pNewElement->GetAttributes().Set(TEXT("width"), pEmoji->Width);
								if (pEmoji->Height != 0)
									pNewElement->GetAttributes().Set(TEXT("height"), pEmoji->Height);
								pNewElement->GetFormat().Align = m_TextFormat.Align;
								pNewElement->SetCharIndex(pElement->GetCharIndex() + j);
								m_HTMLElements.Insert(pNewElement, InsertIndex);
							}
							SrcText.LeftInline(j);
						}
						else
						{
							// 普通TextField不支持表情符，直接移除
							SrcText.RemoveAt(j, 2);
						}
					}
					else
					{
						// 非法的高代理，直接移除
						SrcText.RemoveAt(j, 1);
					}
				}
				else if (SrcText[j] == TEXT('\n'))
				{
					// 拆出换行符作为独立元素，因为UE的字形序列不会包含换行符
					int32 CutLen = SrcText.Len() - j - 1;
					int32 InsertIndex = i + 1;
					if (CutLen > 0)
					{
						auto pNewElement = FHTMLElement::Borrow();
						pNewElement->SetType(pElement->GetType());
						pNewElement->GetFormat() = pElement->GetFormat();
						pNewElement->GetText() = SrcText.Right(CutLen);
						pNewElement->SetCharIndex(pElement->GetCharIndex() + j + 1);
						m_HTMLElements.Insert(pNewElement, InsertIndex);
					}
					{
						auto pNewElement = FHTMLElement::Borrow();
						pNewElement->SetType(EHTMLElementType::NewLine);
						pNewElement->GetFormat() = pElement->GetFormat();
						pNewElement->SetCharIndex(pElement->GetCharIndex() + j);
						m_HTMLElements.Insert(pNewElement, InsertIndex);
					}
					SrcText.LeftInline(j);
				}
				else if (SrcText[j] == TEXT('\r'))
				{
					// 回车符，直接移除
					SrcText.RemoveAt(j, 1);
				}
			}
		}
	}
	if (GObject.IsValid())
	{
		for (auto pElement : m_HTMLElements)
		{
			pElement->CreateHTMLObject(pRich, GObject.Get());
		}
	}
}

void STextField::BuildGlyphs()
{
	ClearGlyphs();

	m_RenderScale = UWidgetLayoutLibrary::GetViewportScale(GObject->GetWorld());
	m_EllipsisCharIndex = -1;

	m_TextWidth = 0;
	m_TextHeight = 0;

	float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;
	float LineSpacing = (m_TextFormat.LineSpacing - 1) * m_FontSizeScale;

	float WidthLimit = GetMaxFieldWidth();

	LineInfo* pCurLine = nullptr;

	FName CurFontName = G_DEFAULT_FONT_NAME;
	if (!m_TextFormat.Face.IsNone())
		CurFontName = m_TextFormat.Face;
	UBaseFont* pCurFont = UUIPackage::GetFontManager()->GetFont(CurFontName);
	pCurFont->SetFormat(m_TextFormat);

	for (auto pElement : m_HTMLElements)
	{
		float LineHeight = pCurFont->GetLineHeight(pElement->GetFormat().Size) * m_FontSizeScale;
		if (pElement->GetType() == EHTMLElementType::Text)
		{
			FName NewFontName = G_DEFAULT_FONT_NAME;
			if (!pElement->GetFormat().Face.IsNone())
				NewFontName = pElement->GetFormat().Face;
			if (CurFontName != NewFontName)
			{
				CurFontName = NewFontName;
				pCurFont = UUIPackage::GetFontManager()->GetFont(CurFontName);
			}
			pCurFont->SetFormat(pElement->GetFormat());
			FString CurText = pElement->GetText();
			auto	pGlyphSequence = pCurFont->GetGlyph(CurText);
			if (!pGlyphSequence)
				continue;
			m_GlyphSequences.Add(pGlyphSequence);
			if (WidthLimit < 0)
			{
				pCurLine = AddCharBlockToLine(
					pCurLine, pElement, pGlyphSequence, pElement->GetCharIndex(), LineHeight, LetterSpacing);
				if (!pCurLine)
					return;
			}
			else
			{
				int32 CharStartIndex = pElement->GetCharIndex();
				int32 BreasPos;
				auto  CurBreaker = GetWordBreaker(m_bWrapBreakByWord);
				do
				{
					BreasPos = FindLineBreak(CurText, pGlyphSequence, LetterSpacing, WidthLimit, CurBreaker);
					if (BreasPos == 0 && (pCurLine == nullptr || pCurLine->Blocks.Num() == 0) && m_bWrapBreakByWord)
					{
						// 按单词断行出现了空行，用按字母断行再试一次
						CurBreaker = GetWordBreaker(false);
						BreasPos = FindLineBreak(CurText, pGlyphSequence, LetterSpacing, WidthLimit, CurBreaker);
					}
					if (BreasPos == INDEX_NONE)
					{
						pCurLine = AddCharBlockToLine(
							pCurLine, pElement, pGlyphSequence, CharStartIndex, LineHeight, LetterSpacing);
						if (!pCurLine)
							return;
					}
					else if (BreasPos == 0)
					{
						// 首元素且字段窄到连一个字符都放不下时 pCurLine 还是 nullptr
						if (pCurLine == nullptr || pCurLine->Blocks.Num() == 0)
						{
							UE_LOG(LogFairyGUI, Warning,
								TEXT("field width(%f) can not accommodate even one character!"), GetWidth());
							break;
						}
						pCurLine = AddNewLine(pCurLine, CharStartIndex, LineHeight);
						WidthLimit = GetMaxFieldWidth();
					}
					else
					{
						m_GlyphSequences.Remove(pGlyphSequence);
						pGlyphSequence->Release();
						pGlyphSequence = pCurFont->GetGlyph(CurText.Left(BreasPos));
						if (!pGlyphSequence)
							break;
						m_GlyphSequences.Add(pGlyphSequence);
						pCurLine = AddCharBlockToLine(
							pCurLine, pElement, pGlyphSequence, CharStartIndex, LineHeight, LetterSpacing);
						if (!pCurLine)
							return;
						CharStartIndex += BreasPos;
						CurText.RightInline(CurText.Len() - BreasPos);
						pGlyphSequence = pCurFont->GetGlyph(CurText);
						if (!pGlyphSequence)
							break;
						m_GlyphSequences.Add(pGlyphSequence);
					}
					WidthLimit = GetMaxFieldWidth() - pCurLine->Width;
				}
				while (BreasPos != INDEX_NONE);
			}
		}
		else if (pElement->GetType() == EHTMLElementType::NewLine)
		{
			if (pCurLine)
				pCurLine->CharCount++;
			pCurLine = AddNewLine(pCurLine, pElement->GetCharIndex() + 1, LineHeight);
			WidthLimit = GetMaxFieldWidth();
		}
		else if ((pElement->GetType() == EHTMLElementType::Image || pElement->GetType() == EHTMLElementType::Input
					 || pElement->GetType() == EHTMLElementType::Select)
			&& pElement->GetHTMLObject())
		{
			pCurLine = AddObjectBlockToLine(pCurLine, pElement, LineHeight, LetterSpacing, WidthLimit);
			if (!pCurLine)
				return;
			WidthLimit = GetMaxFieldWidth() - pCurLine->Width;
		}
	}
	m_TextHeight = GUTTER_Y;
	for (int32 i = 0; i < m_Lines.Num(); i++)
	{
		auto pLineInfo = m_Lines[i];
		if (pLineInfo->Width > m_TextWidth)
			m_TextWidth = pLineInfo->Width;
		if (i)
			m_TextHeight += LineSpacing;
		m_TextHeight += pLineInfo->Height;
		if (m_TextHeight < GUTTER_Y)
			m_TextHeight = GUTTER_Y;
	}
	m_TextWidth += GUTTER_X * 2;
	m_TextHeight += GUTTER_Y;
}

void STextField::UpdateSize()
{
	m_bInUpdateSize = true;
	auto NewSize = m_Size;
	if (m_AutoSize == EAutoSizeType::Both)
	{
		if (IsA<SInputTextField>())
		{
			UBaseFont* pCurFont = UUIPackage::GetFontManager()->GetFont(m_TextFormat.Face);
			float	   w = FMath::Max(m_TextFormat.Size, m_TextWidth);
			float	   h =
				FMath::Max(pCurFont->GetLineHeight(m_TextFormat.Size) * m_FontSizeScale + GUTTER_Y * 2, m_TextHeight);
			NewSize.X = w;
			NewSize.Y = h;
		}
		else
		{
			NewSize.X = m_TextWidth;
			NewSize.Y = m_TextHeight;
		}
	}
	else if (m_AutoSize == EAutoSizeType::Height)
	{
		if (IsA<SInputTextField>())
		{
			UBaseFont* pCurFont = UUIPackage::GetFontManager()->GetFont(m_TextFormat.Face);
			NewSize.Y =
				FMath::Max(pCurFont->GetLineHeight(m_TextFormat.Size) * m_FontSizeScale + GUTTER_Y * 2, m_TextHeight);
		}
		else
		{
			NewSize.Y = m_TextHeight;
		}
	}
	if (!NewSize.Equals(m_Size))
	{
		SetSize(NewSize);
		if (GObject.IsValid())
			GObject->SetSize(NewSize);
	}
	m_bInUpdateSize = false;
}
void STextField::BuildMesh()
{
	m_TextMesh.Reset();
	float LBound = GUTTER_X;
	float RBound = GetWidth() - GUTTER_X;
	float TBound = GUTTER_Y;
	float BBound = GetHeight() - GUTTER_Y;
	float YOffset = TBound + m_TextDrawOffset.Y;
	float LineSpacing = (m_TextFormat.LineSpacing - 1) * m_FontSizeScale;
	if (m_TextHeight < GetHeight())
	{
		switch (m_VerticalAlign)
		{
			case EVerticalAlignType::Top:
				break;
			case EVerticalAlignType::Middle:
				YOffset = (GetHeight() - m_TextHeight) / 2 + GUTTER_Y;
				break;
			case EVerticalAlignType::Bottom:
				YOffset = (GetHeight() - m_TextHeight) + GUTTER_Y;
				break;
		}
	}
	int32 VisibleLines = 0;
	for (auto pLine : m_Lines)
	{
		bool IsOutVBound = false;
		if (m_ClipType == ETextClipType::ByChar)
		{
			if (YOffset < TBound || YOffset + pLine->Height > BBound)
			{
				// 第一行只要部分可见就强制显示
				if (VisibleLines == 0 && YOffset < BBound && YOffset + pLine->Height > TBound)
					IsOutVBound = false;
				else
					IsOutVBound = true;
			}
		}
		else
		{
			if (YOffset + pLine->Height < TBound || YOffset > BBound)
				IsOutVBound = true;
		}
		// 对于SInputTextField，即使超出显示范围了，还必须计算以保证光标计算正确
		if (IsOutVBound && !IsA<SInputTextField>())
			break;
		if (!IsOutVBound)
			VisibleLines++;
		float XMin = LBound + m_TextDrawOffset.X;
		float XMax = RBound + m_TextDrawOffset.X;
		float CenterWidth = 0;
		int32 DrawBlockCount = 0;
		switch (m_TextFormat.Align)
		{
			case EAlignType::Left:
				pLine->X = XMin;
				break;
			case EAlignType::Center:
				pLine->X = (XMin + XMax) / 2;
				break;
			case EAlignType::Right:
				pLine->X = XMax;
				break;
		}
		pLine->Y = YOffset;
		// 先画左对齐
		for (int32 i = 0; i < pLine->Blocks.Num(); i++)
		{
			auto  pBlock = pLine->Blocks[i];
			auto  Align = m_TextFormat.Align;
			float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;
			if (pBlock->pElement)
			{
				Align = pBlock->pElement->GetFormat().Align;
				LetterSpacing = pBlock->pElement->GetFormat().LetterSpacing * m_FontSizeScale;
			}
			if (Align == EAlignType::Left)
			{
				pBlock->X = XMin;
				pBlock->Y = YOffset + (pLine->Baseline - pBlock->Baseline);
				if (!IsOutVBound)
					BuildLineBlockMesh(m_TextMesh, pBlock, pBlock->X, LBound, RBound, pBlock->Y, LetterSpacing);
				XMin += pBlock->Width;
				DrawBlockCount++;
				if (DrawBlockCount)
					XMin += LetterSpacing;
			}
			else if (Align == EAlignType::Center)
			{
				if (CenterWidth)
					CenterWidth += pBlock->Width + LetterSpacing;
				else
					CenterWidth += pBlock->Width;
			}
		}
		// 然后右对齐
		DrawBlockCount = 0;
		for (int32 i = pLine->Blocks.Num() - 1; i >= 0; i--)
		{
			auto  pBlock = pLine->Blocks[i];
			auto  Align = m_TextFormat.Align;
			float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;
			if (pBlock->pElement)
			{
				Align = pBlock->pElement->GetFormat().Align;
				LetterSpacing = pBlock->pElement->GetFormat().LetterSpacing * m_FontSizeScale;
			}
			if (Align == EAlignType::Right)
			{
				XMax -= pBlock->Width;
				if (DrawBlockCount)
					XMax -= LetterSpacing;
				pBlock->X = XMax;
				pBlock->Y = YOffset + (pLine->Baseline - pBlock->Baseline);
				if (!IsOutVBound)
					BuildLineBlockMesh(m_TextMesh, pBlock, pBlock->X, LBound, RBound, pBlock->Y, LetterSpacing);
				DrawBlockCount++;
			}
		}
		// 最后居中的
		DrawBlockCount = 0;
		XMin += FMath::Max((XMax - CenterWidth - XMin) / 2, 0);
		for (int32 i = 0; i < pLine->Blocks.Num(); i++)
		{
			auto  pBlock = pLine->Blocks[i];
			auto  Align = m_TextFormat.Align;
			float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;
			if (pBlock->pElement)
			{
				Align = pBlock->pElement->GetFormat().Align;
				LetterSpacing = pBlock->pElement->GetFormat().LetterSpacing * m_FontSizeScale;
			}
			if (Align == EAlignType::Center)
			{
				pBlock->X = XMin;
				pBlock->Y = YOffset + (pLine->Baseline - pBlock->Baseline);
				if (!IsOutVBound)
					BuildLineBlockMesh(m_TextMesh, pBlock, pBlock->X, LBound, RBound, pBlock->Y, LetterSpacing);
				XMin += pBlock->Width;
				DrawBlockCount++;
				if (DrawBlockCount)
					XMin += LetterSpacing;
			}
		}
		YOffset += pLine->Height + LineSpacing;
	}
	m_TextMesh.RemoveEmptySubMeshs();
}

void STextField::BuildLineBlockMesh(
	FTextMesh& TextMesh, LineBlock* pBlock, float X, float LBound, float RBound, float Y, float LetterSpacing)
{
	if (pBlock->pGlyphSequence)
	{
		int32 StartChar = 0;
		int32 CharCount = pBlock->Chars.Num();
		if (StartChar >= CharCount)
			return;
		// 块尾部超出右界就需要裁剪（多块行中每个块都不宽，但起点靠后时会超出）
		if (X + pBlock->Width > RBound)
		{
			int32 EndChar = StartChar;
			float EndX = X;
			for (; EndChar < CharCount; EndChar++)
			{
				auto Char = pBlock->Chars[EndChar];
				EndX += Char->Width;
				if (EndX > RBound)
				{
					if (m_ClipType == ETextClipType::ByChar)
						EndChar--;
					break;
				}
				EndX += LetterSpacing;
			}
			if (EndChar < StartChar)
				return;
			if (EndChar >= CharCount)
				EndChar = CharCount - 1;
			CharCount = EndChar - StartChar + 1;
		}
		pBlock->pGlyphSequence->DrawGlyph(StartChar, CharCount, TextMesh, X, Y, LetterSpacing, m_FontSizeScale);
	}
}
void STextField::UpdateHTMLObjects()
{
	UHTMLLink* pLink = nullptr;
	for (auto pElement : m_HTMLElements)
	{
		if (pElement->GetHTMLObject())
			pElement->GetHTMLObject()->Remove();

		if (pElement->GetType() == EHTMLElementType::Link)
		{
			pLink = Cast<UHTMLLink>(pElement->GetHTMLObject());
		}
		else if (pElement->GetType() == EHTMLElementType::LinkEnd && pLink)
		{
			pLink->SetPosition(0, 0);
			pLink->SetSize(GetWidth(), GetHeight());
			pLink->SetArea(pLink->GetHTMLElement()->GetCharIndex(), pElement->GetCharIndex());
			pLink->Add();
			pLink = nullptr;
		}
	}
	for (auto pLine : m_Lines)
	{
		for (auto pBlock : pLine->Blocks)
		{
			if (pBlock->pElement && pBlock->pElement->GetHTMLObject())
			{
				pBlock->pElement->GetHTMLObject()->SetPosition(pBlock->X + 1, pBlock->Y);
				pBlock->pElement->GetHTMLObject()->Add();
			}
		}
	}
}

void STextField::DoShrink()
{
	if (m_Lines.Num() > 1 && m_TextHeight > GetHeight())
	{
		// 多行的情况，涉及到自动换行，得用二分法查找最合适的比例，会消耗多一点计算资源
		int32 Low = 0;
		int32 High = m_TextFormat.Size;

		// 先尝试猜测一个比例
		m_FontSizeScale = FMath::Sqrt(GetHeight() / m_TextHeight);
		int32 Cur = FMath::FloorToInt(m_FontSizeScale * m_TextFormat.Size);

		while (true)
		{
			LineInfo::Return(m_Lines);
			BuildGlyphs();

			if (m_TextWidth > GetWidth() || m_TextHeight > GetHeight())
				High = Cur;
			else
				Low = Cur;
			if (High - Low > 1 || (High != Low && Cur == High))
			{
				Cur = Low + (High - Low) / 2;
				m_FontSizeScale = (float)Cur / m_TextFormat.Size;
			}
			else
				break;
		}
	}
	else if (m_TextWidth > GetWidth())
	{
		m_FontSizeScale = GetWidth() / m_TextWidth;

		LineInfo::Return(m_Lines);
		BuildGlyphs();

		if (m_TextWidth > GetWidth()) // 如果还超出，缩小一点再来一次
		{
			int32 CurSize = FMath::FloorToInt(m_TextFormat.Size * m_FontSizeScale);
			CurSize--;
			m_FontSizeScale = (float)CurSize / m_TextFormat.Size;

			LineInfo::Return(m_Lines);
			BuildGlyphs();
		}
	}
}
void STextField::DoEllipsis()
{
	float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;
	float LineSpacing = (m_TextFormat.LineSpacing - 1) * m_FontSizeScale;
	// 可用文本区域需扣除四周 GUTTER，与 GetMaxFieldWidth/BuildMesh 的排版边界保持一致
	float FieldWidth = GetWidth() - GUTTER_X * 2;
	float FieldHeight = GetHeight() - GUTTER_Y * 2;
	if (m_Lines.Num())
	{
		// 逐行累加高度（行间有 LineSpacing），找出能放进 FieldHeight 的行
		int32 OriginalLineCount = m_Lines.Num();
		auto pLineInfo = m_Lines[0];
		{
			float Height = pLineInfo->Height;
			int32 i = 1;
			for (; i < m_Lines.Num(); i++)
			{
				if (Height + LineSpacing + m_Lines[i]->Height > FieldHeight)
					break;
				pLineInfo = m_Lines[i];
				Height += LineSpacing + pLineInfo->Height;
			}
			for (int32 j = i; j < m_Lines.Num(); j++)
				LineInfo::Return(m_Lines[j]);
			m_Lines.SetNum(i);
		}
		// 裁掉了后续行时，最后一行也要显示省略号（行本身不超宽，但内容被裁）
		bool bLineTrimmed = m_Lines.Num() < OriginalLineCount;
		if (pLineInfo->Width > FieldWidth || bLineTrimmed)
		{
			FName CurFontName = G_DEFAULT_FONT_NAME;
			if (!m_TextFormat.Face.IsNone())
				CurFontName = m_TextFormat.Face;
			UBaseFont* pFont = UUIPackage::GetFontManager()->GetFont(CurFontName);
			pFont->SetFormat(m_TextFormat);
			float LineHeight = pFont->GetLineHeight(m_TextFormat.Size) * m_FontSizeScale;
			auto  pEllipsisGlyphSequence = pFont->GetGlyph(ELLIPSIS_TEXT);
			if (!pEllipsisGlyphSequence)
				return;
			m_GlyphSequences.Add(pEllipsisGlyphSequence);
			float Width = 0;
			// 累计省略号自身宽度（含内部字间距）
			for (int32 i = 0; i < pEllipsisGlyphSequence->GetGlyphCount(); i++)
			{
				float GlyphWidth, GlyphHeight;
				int32 CharIndex;
				if (pEllipsisGlyphSequence->GetGlyph(i, GlyphWidth, GlyphHeight, CharIndex))
				{
					Width += GlyphWidth * m_FontSizeScale;
					if (i)
						Width += LetterSpacing;
				}
			}
			// 另预留省略号与前面内容之间的字间距
			float EllipsisWidth = Width + LetterSpacing;
			int32 BlockIndex = 0;
			int32 CharIndex = 0;
			bool bOver = false;
			while (BlockIndex < pLineInfo->Blocks.Num())
			{
				auto pBlock = pLineInfo->Blocks[BlockIndex];
				for (CharIndex = 0; CharIndex < pBlock->Chars.Num(); CharIndex++)
				{
					Width += pBlock->Chars[CharIndex]->Width;
					if (BlockIndex || CharIndex)
						Width += LetterSpacing;
					if (Width + EllipsisWidth > FieldWidth)
					{
						// 当前字符放不下省略号，从这里截断（含当前字符之前的内容）
						bOver = true;
						break;
					}
				}
				if (bOver || CharIndex < pBlock->Chars.Num())
					break;
				BlockIndex++;
			}
			for (int32 i = BlockIndex + 1; i < pLineInfo->Blocks.Num(); i++)
				LineBlock::Return(pLineInfo->Blocks[i]); // Clear 会连同 Chars 一起归还
			// 逐字符口径下整行 + 省略号都放得下（行宽判定超出是尾随字间距口径差），无需截断
			bool bSkipTruncate = false;
			if (!bOver && BlockIndex >= pLineInfo->Blocks.Num())
			{
				// 因裁行进入本分支的，行内容无需截断，直接在行尾追加省略号
				if (!bLineTrimmed)
					return;
				bSkipTruncate = true;
			}
			if (!bSkipTruncate && BlockIndex < pLineInfo->Blocks.Num())
			{
				auto pBlock = pLineInfo->Blocks[BlockIndex];
				if (CharIndex)
				{
					// 归还被截掉的字符（从 CharIndex 到末尾），保留块
					for (int32 i = CharIndex; i < pBlock->Chars.Num(); i++)
						LineCharInfo::Return(pBlock->Chars[i]);
					pBlock->Chars.SetNum(CharIndex);
					pLineInfo->Blocks.SetNum(BlockIndex + 1);
				}
				else
				{
					// 第一个字符就放不下，整块移除（Clear 会归还块内全部字符）
					LineBlock::Return(pBlock);
					pLineInfo->Blocks.SetNum(BlockIndex);
				}
			}
			AddCharBlockToLine(pLineInfo, nullptr, pEllipsisGlyphSequence, 0, LineHeight, LetterSpacing);
			pLineInfo->Width = 0;
			pLineInfo->Height = 0;
			pLineInfo->Baseline = 0;
			for (int32 i = 0; i < pLineInfo->Blocks.Num(); i++)
			{
				auto pBlock = pLineInfo->Blocks[i];
				// 截断后块的宽度也是旧值，必须重算，否则 BuildMesh 用它推进 X 会把省略号推出框外
				pBlock->Width = 0;
				for (int32 j = 0; j < pBlock->Chars.Num(); j++)
				{
					auto pChar = pBlock->Chars[j];
					pBlock->Width += pChar->Width;
					if (j)
						pBlock->Width += LetterSpacing;
					pLineInfo->Width += pChar->Width;
					if (i || j)
						pLineInfo->Width += LetterSpacing;
					if (pChar->Height > pLineInfo->Height)
						pLineInfo->Height = pChar->Height;
				}
				// 省略号块基线与原文字符混合对齐
				if (pBlock->Baseline > pLineInfo->Baseline)
					pLineInfo->Baseline = pBlock->Baseline;
			}
		}
		// 裁剪后按剩余行重算文本总宽高，供 BuildMesh 垂直对齐使用
		m_TextWidth = 0;
		m_TextHeight = GUTTER_Y;
		for (int32 i = 0; i < m_Lines.Num(); i++)
		{
			auto pLine = m_Lines[i];
			if (pLine->Width > m_TextWidth)
				m_TextWidth = pLine->Width;
			if (i)
				m_TextHeight += LineSpacing;
			m_TextHeight += pLine->Height;
		}
		m_TextWidth += GUTTER_X * 2;
		m_TextHeight += GUTTER_Y;
	}
}

STextField::LineInfo* STextField::AddCharBlockToLine(LineInfo* pLine, FHTMLElement* pElement,
	TSharedPtr<FBaseGlyphSequence> pGlyphSequence, int32 StartCharIndex, float LineHeight, float LetterSpacing)
{
	if (pLine == nullptr)
	{
		pLine = LineInfo::Borrow();
		if (!pLine)
			return nullptr;
		pLine->Height = LineHeight;
		pLine->CharIndex = StartCharIndex;
		m_Lines.Add(pLine);
	}
	LineBlock* pBlock = LineBlock::Borrow();
	if (!pBlock)
		return nullptr;
	pLine->Blocks.Add(pBlock);
	pBlock->pElement = pElement;
	pBlock->CharIndex = StartCharIndex;
	pBlock->pGlyphSequence = pGlyphSequence;
	pBlock->Baseline = pGlyphSequence->GetBaseline(m_FontSizeScale);
	for (int32 i = 0; i < pGlyphSequence->GetGlyphCount(); i++)
	{
		float GlyphWidth;
		float GlyphHeight;
		int32 CharIndex;
		if (pGlyphSequence->GetGlyph(i, GlyphWidth, GlyphHeight, CharIndex))
		{
			GlyphWidth *= m_FontSizeScale;
			GlyphHeight *= m_FontSizeScale;
			LineCharInfo* pChar = LineCharInfo::Borrow();
			if (!pChar)
				return nullptr;
			pChar->Width = GlyphWidth;
			pChar->Height = GlyphHeight;
			pChar->GlyphIndex = i;
			pChar->CharIndex = StartCharIndex + CharIndex;
			pBlock->Chars.Add(pChar);
			pLine->CharCount++;
			pBlock->Width += GlyphWidth;
			if (pBlock->Chars.Num() > 1)
				pBlock->Width += LetterSpacing;
			if (GlyphHeight > pBlock->Height)
				pBlock->Height = GlyphHeight;
			pLine->Width += GlyphWidth;
			if (pLine->Blocks.Num() > 1 || pBlock->Chars.Num() > 1)
				pLine->Width += LetterSpacing;
			if (GlyphHeight > pLine->Height)
				pLine->Height = GlyphHeight;
		}
	}
	// 更新行基线为所有块基线的最大值，实现混合字号基线对齐
	if (pBlock->Baseline > pLine->Baseline)
		pLine->Baseline = pBlock->Baseline;
	return pLine;
}
STextField::LineInfo* STextField::AddObjectBlockToLine(
	LineInfo* pLine, FHTMLElement* pElement, float LineHeight, float LetterSpacing, float LenLimit)
{
	auto pHTMLObject = pElement->GetHTMLObject();
	if (!pHTMLObject)
		return nullptr;
	if (pLine == nullptr)
	{
		pLine = LineInfo::Borrow();
		if (!pLine)
			return nullptr;
		pLine->Height = LineHeight;
		pLine->CharIndex = pElement->GetCharIndex();
		m_Lines.Add(pLine);
	}
	float ObjWidth = pHTMLObject->GetWidth() + 2;
	float ObjHeight = pHTMLObject->GetHeight();
	if (LenLimit >= 0)
	{
		if (ObjWidth + (pLine->Blocks.Num() ? LetterSpacing : 0) > LenLimit)
			pLine = AddNewLine(pLine, pElement->GetCharIndex(), LineHeight);
	}
	LineBlock* pBlock = LineBlock::Borrow();
	if (!pBlock)
		return nullptr;
	pLine->Blocks.Add(pBlock);
	pBlock->pElement = pElement;
	pBlock->CharIndex = pElement->GetCharIndex();
	pBlock->Baseline = ObjHeight * HTML_OBJ_BASELINE;
	// 更新行基线为所有块基线的最大值
	if (pBlock->Baseline > pLine->Baseline)
	{
		float OldBaseline = pLine->Baseline;
		pLine->Baseline = pBlock->Baseline;
		if (pLine->Blocks.Num() > 1)
		{
			pLine->Height += pLine->Baseline - OldBaseline;
		}
	}
	auto pChar = LineCharInfo::Borrow();
	if (!pChar)
		return nullptr;
	pChar->CharIndex = pElement->GetCharIndex();
	pChar->Width = ObjWidth;
	pChar->Height = ObjHeight;
	pBlock->Chars.Add(pChar);
	pLine->CharCount++;
	pBlock->Width += ObjWidth;
	if (ObjHeight > pBlock->Height)
		pBlock->Height = ObjHeight;
	pLine->Width += ObjWidth;
	if (pLine->Blocks.Num() > 1)
		pLine->Width += LetterSpacing;
	// 考虑基线偏移后的有效底部位置
	float EffectiveBottom = (pLine->Baseline - pBlock->Baseline) + ObjHeight;
	if (EffectiveBottom > pLine->Height)
		pLine->Height = EffectiveBottom;
	return pLine;
}

STextField::LineInfo* STextField::AddNewLine(LineInfo* pLine, int32 StartCharIndex, float LineHeight)
{
	if (pLine == nullptr)
	{
		pLine = LineInfo::Borrow();
		if (!pLine)
			return nullptr;
		pLine->Height = LineHeight;
		m_Lines.Add(pLine);
	}
	LineInfo* pNewLine = LineInfo::Borrow();
	if (!pNewLine)
		return nullptr;
	pNewLine->Height = LineHeight;
	pNewLine->CharIndex = StartCharIndex;
	m_Lines.Add(pNewLine);
	return pNewLine;
}
int32 STextField::FindLineBreak(const FString& SourceText, TSharedPtr<FBaseGlyphSequence> pGlyphSequence, float LetterSpacing,
	float LenLimit, TSharedRef<IBreakIterator>& Breader)
{
	TArray<int32> GlyphIndices;
	GlyphIndices.Init(-1, SourceText.Len());
	pGlyphSequence->GetCharToGlyphMap(GlyphIndices);
	int32 BreakPos = 0;
	float CurLen = 0;
	Breader->SetString(SourceText);
	Breader->ResetToBeginning();
	while (true)
	{
		int32 NextBreak = Breader->MoveToNext();
		if (NextBreak == INDEX_NONE)
			break;
		for (int32 i = BreakPos; i < NextBreak; i++)
		{
			int32 GlyphIndex = GlyphIndices[i];
			float GlyphWidth;
			float GlyphHeight;
			int32 CharIndex;
			if (pGlyphSequence->GetGlyph(GlyphIndex, GlyphWidth, GlyphHeight, CharIndex))
			{
				GlyphWidth *= m_FontSizeScale;
				if (i)
					GlyphWidth += LetterSpacing;
				if (CurLen + GlyphWidth > LenLimit)
					return BreakPos;
				CurLen += GlyphWidth;
			}
		}
		BreakPos = NextBreak;
	}
	return INDEX_NONE;
}
int32 STextField::GetMaxFieldWidth()
{
	if (m_bSingleLine)
	{
		return -1;
	}
	else if (m_AutoSize == EAutoSizeType::Both)
	{
		if (m_MaxWidth)
			return m_MaxWidth;
		return -1;
	}
	else
	{
		return GetWidth() - GUTTER_X * 2;
	}
}

void STextField::OnFontAtlasReleased(const FSlateFontCache& FontCache)
{
	m_bRebuildMesh = true;
}

TSharedRef<IBreakIterator> STextField::GetWordBreaker(bool bWrapBreakByWord)
{
	static TSharedRef<IBreakIterator> BREAKER_BY_WORD(FBreakIterator::CreateLineBreakIterator());
	static TSharedRef<IBreakIterator> BREAKER_BY_CHAR(FBreakIterator::CreateCharacterBoundaryIterator());
	return bWrapBreakByWord ? BREAKER_BY_WORD : BREAKER_BY_CHAR;
}