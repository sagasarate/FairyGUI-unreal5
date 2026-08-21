#pragma once

#include "SDisplayObject.h"
#include "UI/FieldTypes.h"
// #include "Mesh/MeshFactory.h"
#include "Font/NTextFormat.h"
#include "Utils/HTML/HTMLElement.h"
#include "Utils/IDStorage.h"
#include "Utils/TypeID.h"
#include "Widgets/Mesh/TextMesh.h"

class UBaseFont;
class FBaseGlyphSequence;

enum class ETextClipType
{
	ByChar,
	ByPixel,
};

inline bool IsHighSurrogate(TCHAR Char)
{
	return (Char & 0xFC00) == 0xD800;
}

// 判断是否为低位代理
inline bool IsLowSurrogate(TCHAR Char)
{
	return (Char & 0xFC00) == 0xDC00;
}
inline bool IsArabicLetter(TCHAR Char)
{
	// 基础阿拉伯语区间 (0x0600 - 0x06FF)
	// 阿拉伯语补充区间 (0x0750 - 0x077F)
	return (Char >= 0x0600 && Char <= 0x06FF) || (Char >= 0x0750 && Char <= 0x077F);
}

class FAIRYGUI_API STextField : public SDisplayObject
{
protected:
	struct LineCharInfo
	{
		uint32 PoolID;
		float  Width;
		float  Height;
		int32  CharIndex;
		int32  GlyphIndex;

		LineCharInfo() { Clear(); }

		void Clear()
		{
			PoolID = 0;
			Width = 0;
			Height = 0;
			CharIndex = 0;
			GlyphIndex = 0;
		}
		void SetID(uint32 ID) { PoolID = ID; }

		static CIDStorage<LineCharInfo> Pool;

		static LineCharInfo* Borrow();
		static void			 Return(LineCharInfo* pValue);
		static void			 Return(TArray<LineCharInfo*>& Values);
	};

	struct LineBlock
	{
		uint32						   PoolID;
		FHTMLElement*				   pElement;
		float						   Baseline;
		float						   Width;
		float						   Height;
		float						   X;
		float						   Y;
		int32						   CharIndex;
		TSharedPtr<FBaseGlyphSequence> pGlyphSequence;
		TArray<LineCharInfo*>		   Chars;

		LineBlock() { Clear(); }
		void Clear();
		void SetID(uint32 ID) { PoolID = ID; }

		static CIDStorage<LineBlock> Pool;

		static LineBlock* Borrow();
		static void		  Return(LineBlock* pValue);
		static void		  Return(TArray<LineBlock*>& Values);
	};

	typedef LineCharInfo* PLineCharInfo;

	struct LineInfo
	{
		uint32 PoolID;
		float  Baseline;
		float  Width;
		float  Height;
		float  X;
		float  Y;
		int32  CharIndex;
		int32  CharCount;

		TArray<LineBlock*> Blocks;

		LineInfo() { Clear(); }

		void Clear();

		void SetID(uint32 ID) { PoolID = ID; }

		static CIDStorage<LineInfo> Pool;

		static LineInfo* Borrow();
		static void		 Return(LineInfo* pValue);
		static void		 Return(TArray<LineInfo*>& Values);
	};

protected:
	FString m_Text;

	FNTextFormat m_TextFormat;

	EVerticalAlignType m_VerticalAlign = EVerticalAlignType::Top;
	EAutoSizeType	   m_AutoSize = EAutoSizeType::None;
	ETextClipType	   m_ClipType = ETextClipType::ByChar;

	bool m_bInput = false;
	bool m_bSingleLine = false;
	bool m_bWrapBreakByWord = true;
	bool m_bUBBEnabled = false;

	bool m_bRebuildText = false;
	bool m_bRebuildGlyph = false;
	bool m_bInUpdateSize = false;
	bool m_IsFontAtlasReleasedRegistered = false;

	float m_MaxWidth = 0;
	float m_TextWidth = 0;
	float m_TextHeight = 0;
	float m_FontSizeScale = 1.0f;
	float m_RenderScale = 1.0f;
	int	  m_EllipsisCharIndex = 0;
	int	  m_TypingEffectPos = 0;

	FVector2D m_TextDrawOffset = FVector2D::ZeroVector;

	mutable FTextMesh m_TextMesh;

	TArray<FHTMLElement*>				   m_HTMLElements;
	TArray<LineInfo*>					   m_Lines;
	TArray<TSharedPtr<FBaseGlyphSequence>> m_GlyphSequences;

	mutable TArray<FSlateVertex> m_VertexCache;

	static FString ELLIPSIS_TEXT;

public:
	DECLARE_TYPE_ID(STextField, SDisplayObject)
	static const int32 GUTTER_X;
	static const int32 GUTTER_Y;
	static const float HTML_OBJ_BASELINE;
	static const int32 ELLIPSIS_LENGTH;

	explicit STextField(UGObject* InGObject = nullptr);
	virtual ~STextField() override;

	const FString& GetText() const { return m_Text; }
	void		   SetText(const FString& InText);

	EAutoSizeType GetAutoSize() const { return m_AutoSize; };
	void		  SetAutoSize(EAutoSizeType InAutoSize);

	EAlignType GetAlign() { return m_TextFormat.Align; }
	void	   SetAlign(EAlignType Align);

	EVerticalAlignType GetVerticalAlign() { return m_VerticalAlign; }
	void			   SetVerticalAlign(EVerticalAlignType Align);

	bool IsSingleLine() const { return m_bSingleLine; }
	void SetSingleLine(bool InSingleLine);

	FNTextFormat& GetTextFormat() { return m_TextFormat; }
	virtual void  SetTextFormat(const FNTextFormat& InFormat);

	float GetMaxWidth() const { return m_MaxWidth; }
	void  SetMaxWidth(float InMaxWidth);

	FVector2D GetTextSize();

	void EnableWrapBreakByWord(bool beEnable);
	bool IsWrapBreakByWord() { return m_bWrapBreakByWord; }

	void		  SetTextClipType(ETextClipType InClipType);
	ETextClipType GetTextClipType() { return m_ClipType; }

	void SetUBBEnabled(bool bEnabled);
	bool IsUBBEnabled() const { return m_bUBBEnabled; }

	void GetLinesShape(int32 StartCharIndex, int32 EndCharIndex, bool bClipped, TArray<FBox2D>& ResultRects);

	void ReBuildText() { m_bRebuildText = true; }
	void ReDraw() { m_bRebuildMesh = true; }

protected:
	virtual void CollectRenderUnits(
		FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha, float InSaturation) const override;

	void ClearGlyphs();
	void ClearAll();

	virtual void BuildLines();

	void ParseText(const FString& Text, bool bEnableUBBParser, bool bEnableHTMLParser);
	void BuildGlyphs();
	void UpdateSize();
	void BuildMesh();
	void BuildLineBlockMesh(
		FTextMesh& TextMesh, LineBlock* pBlock, float X, float LBound, float RBound, float Y, float LetterSpacing);
	void	  UpdateHTMLObjects();
	void	  DoShrink();
	void	  DoEllipsis();
	LineInfo* AddCharBlockToLine(LineInfo* pLine, FHTMLElement* pElement, TSharedPtr<FBaseGlyphSequence> pGlyphSequence,
		int32 StartCharIndex, float LineHeight, float LetterSpacing);
	LineInfo* AddObjectBlockToLine(
		LineInfo* pLine, FHTMLElement* pElement, float LineHeight, float LetterSpacing, float LenLimit);
	LineInfo* AddNewLine(LineInfo* pLine, int32 StartCharIndex, float LineHeight);
	int32	  FindLineBreak(const FString& SourceText, TSharedPtr<FBaseGlyphSequence> pGlyphSequence, float LetterSpacing,
			float LenLimit, TSharedRef<IBreakIterator>& Breader);
	int32	  GetMaxFieldWidth();
	void	  OnFontAtlasReleased(const FSlateFontCache& FontCache);

	static TSharedRef<IBreakIterator> GetWordBreaker(bool bWrapBreakByWord);
};