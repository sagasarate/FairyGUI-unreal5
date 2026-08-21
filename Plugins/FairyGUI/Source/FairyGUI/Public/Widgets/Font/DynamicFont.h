#pragma once

#include "CoreMinimal.h"
#include "Widgets/Font/BaseFont.h"
#include "Utils/TypeID.h"
#include "DynamicFont.generated.h"

class UDynamicFont;

enum EFontTypeFace : uint32
{
	None = 0,
	Bold = 2,
	Italic = 4,
};
ENUM_CLASS_FLAGS(EFontTypeFace)

class FDynamicFontGlyphSequence : public FBaseGlyphSequence
{
protected:
	FNTextFormat			m_TextFormat;
	FShapedGlyphSequencePtr m_ShapedSequence;
	FShapedGlyphSequencePtr m_OutlineShapedSequence;
	//FString					m_Text;
	float					m_RefSdfPixelSpread = 0.0f; // 探针字形的1/SizeU_UV，同字号所有字符共用

public:
	DECLARE_TYPE_ID(FDynamicFontGlyphSequence, FBaseGlyphSequence)

	void SetShapedSequence(FShapedGlyphSequencePtr ShapedSequence) { m_ShapedSequence = ShapedSequence; }
	void SetOutlineShapedSequence(FShapedGlyphSequencePtr OutlineShapedSequence)
	{
		m_OutlineShapedSequence = OutlineShapedSequence;
	}
	FShapedGlyphSequencePtr GetShapedSequence() { return m_ShapedSequence; }
	void					SetTextFormat(const FNTextFormat& Format) { m_TextFormat = Format; }
	const FNTextFormat&		GetTextFormat() { return m_TextFormat; }

	virtual int32 GetGlyphCount() override;
	virtual float GetBaseline(float Scale) override;
	virtual bool  GetGlyph(int32 GlyphIndex, float& GlyphWidth, float& GlyphHeight, int32& CharIndex) override;
	virtual bool  DrawGlyph(
		 int32 Start, int32 Count, FTextMesh& TextMesh, float X, float Y, float LetterSpacing, float Scale) override;
	virtual void GetCharToGlyphMap(TArray<int32>& Map) override;
	virtual void Clear() override;

	bool DrawGlyph(TSharedPtr<FSlateFontCache> FontCache, const FShapedGlyphEntry& Glyph, FTextMesh& TextMesh, float X,
		float Y, float Scale, float Skew, int32 OutlineSize, const FColor& Color, ETextSubMeshType Type,
		ETextDrawType& DrawType, int32& TextureIndex, float SdfBias = 0.5f,
		const FColor& InOutlineColor = FColor::Transparent, float InOutlineBias = 0.5f);
	bool DrawLine(FTextMesh& TextMesh, const FBox2D& DrawRect, int32 TextureIndex, const FBox2D& UVRect,
		const FColor& Color, ETextSubMeshType Type, ETextDrawType DrawType);

	//void SetText(const FString& Text) { m_Text = Text; }
};

UCLASS()
class FAIRYGUI_API UDynamicFont : public UBaseFont
{
	GENERATED_BODY()
protected:
	UPROPERTY()
	FNTextFormat m_TextFormat;
	UPROPERTY()
	FSlateFontInfo m_FontInfo;
	EFontTypeFace  m_FontTypeFace = EFontTypeFace::None;
	EFontTypeFace  m_FontTypeFaceSet = EFontTypeFace::None;

	TArray<TSharedPtr<FDynamicFontGlyphSequence>> m_GlyphPool;

public:
	static const float SUP_SCALE;
	static const float SUP_OFFSET;
	static const float LINE_HEIGHT_FACTOR;

	UDynamicFont();

	bool		  SetFont(UFont* Font, FName FontName = NAME_None);
	bool		  LoadFont(const FString& FontAssetPath, FName FontName = NAME_None);
	EFontTypeFace GetFontTypeFaceSet() { return m_FontTypeFaceSet; }

	virtual void						   SetFormat(const FNTextFormat& Format) override;
	virtual TSharedPtr<FBaseGlyphSequence> GetGlyph(const FString& Text) override;
	virtual bool						   HasCharacter(TCHAR ch) override;
	virtual int							   GetLineHeight(int Size) override;
	virtual bool						   ReturnGlyph(TSharedPtr<FBaseGlyphSequence> pGlyphSequence) override;

	static void SaveFontAtlasToFile(int32 TextureIndex, const FString& FilePath);

protected:
	void OnFontAtlasReleased(const FSlateFontCache& FontCache);
	void UpdateLineTexture();
};
