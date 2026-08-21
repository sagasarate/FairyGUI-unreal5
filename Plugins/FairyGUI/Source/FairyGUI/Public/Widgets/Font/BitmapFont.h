#pragma once

#include "CoreMinimal.h"
#include "Widgets/Font/BaseFont.h"
#include "Utils/TypeID.h"
#include "BitmapFont.generated.h"

class UNTexture;
class UBitmapFont;

struct FBMPFontGlyph
{
	FVector2D Offset;
	FVector2D Size;
	float	  XAdvance;
	float	  LineHeight;
	FBox2D	  UVRect;
	int32	  Channel;
};

class FBMPFontGlyphSequence : public FBaseGlyphSequence
{
protected:
	FNTextFormat		   m_TextFormat;
	TArray<FBMPFontGlyph*> m_Glyphs;
	TArray<int32>		   m_CharIndices;
	float				   m_Scale;

public:
	DECLARE_TYPE_ID(FBMPFontGlyphSequence, FBaseGlyphSequence)

	void				SetTextFormat(const FNTextFormat& Format) { m_TextFormat = Format; }
	const FNTextFormat& GetTextFormat() { return m_TextFormat; }
	void				SetGlyphs(const TArray<FBMPFontGlyph*>& InGlyphs, const TArray<int32>& InCharIndices)
	{
		m_Glyphs = InGlyphs;
		m_CharIndices = InCharIndices;
	}
	void  SetScale(float InScale) { m_Scale = InScale; }
	float GetScale() const { return m_Scale; }

	virtual int32 GetGlyphCount() override;
	virtual float GetBaseline(float Scale) override;
	virtual bool  GetGlyph(int32 GlyphIndex, float& GlyphWidth, float& GlyphHeight, int32& CharIndex) override;
	virtual bool  DrawGlyph(
		 int32 Start, int32 Count, FTextMesh& TextMesh, float X, float Y, float LetterSpacing, float Scale) override;
	virtual void GetCharToGlyphMap(TArray<int32>& Map) override;
	virtual void Clear() override;
};

UCLASS()
class FAIRYGUI_API UBitmapFont : public UBaseFont
{
	GENERATED_BODY()

protected:
	// 原始字体大小
	int32 m_Size = 0;
	// 是否可缩放
	bool m_bResizable = true;
	// 是否使用 BMFont 通道 (R/G/B/A)
	bool m_bHasChannel = false;
	// 是否允许染色
	bool m_bCanTint = true;
	// 字符->字形映射表
	TMap<int32, FBMPFontGlyph> m_GlyphDict;
	// 当前文本格式
	FNTextFormat m_TextFormat;
	// 当前渲染缩放
	float m_Scale = 1.0f;
	// 字形序列对象池
	TArray<TSharedPtr<FBMPFontGlyphSequence>> m_GlyphPool;
	// 字体纹理
	UPROPERTY()
	TObjectPtr<UNTexture> m_Texture;

public:
	UBitmapFont();

	// 字形管理
	void AddChar(TCHAR ch, const FBMPFontGlyph& Glyph);
	void RemoveChar(TCHAR ch);
	void ClearChars();

	// 属性访问器
	void							  SetSize(int32 Size) { m_Size = Size; }
	int32							  GetSize() const { return m_Size; }
	void							  SetResizable(bool bResizable) { m_bResizable = bResizable; }
	bool							  IsResizable() const { return m_bResizable; }
	void							  SetHasChannel(bool bHasChannel) { m_bHasChannel = bHasChannel; }
	bool							  HasChannel() const { return m_bHasChannel; }
	void							  SetCanTint(bool bCanTint) { m_bCanTint = bCanTint; }
	bool							  CanTint() const { return m_bCanTint; }
	void							  SetTexture(UNTexture* Texture) { m_Texture = Texture; }
	UNTexture*						  GetTexture() const { return m_Texture.Get(); }
	const TMap<int32, FBMPFontGlyph>& GetGlyphDict() const { return m_GlyphDict; }

	// UBaseFont 接口
	virtual void						   SetFormat(const FNTextFormat& Format) override;
	virtual TSharedPtr<FBaseGlyphSequence> GetGlyph(const FString& Text) override;
	virtual bool						   HasCharacter(TCHAR ch) override;
	virtual int							   GetLineHeight(int Size) override;
	virtual bool						   ReturnGlyph(TSharedPtr<FBaseGlyphSequence> pGlyphSequence) override;
};