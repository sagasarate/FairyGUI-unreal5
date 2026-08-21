#pragma once

#include "CoreMinimal.h"
#include "Widgets/Font/NTextFormat.h"
#include "Widgets/Mesh/TextMesh.h"
#include "BaseFont.generated.h"

extern FName G_DEFAULT_FONT_NAME;

class UBaseFont;

class FBaseGlyphSequence : public TSharedFromThis<FBaseGlyphSequence>
{
protected:
	uint32					  m_PoolID = 0;
	TWeakObjectPtr<UBaseFont> m_ParentFont;

public:
	static FName			s_TypeName;
	virtual FName			GetTypeName() const { return s_TypeName; }
	static FName			GetTypeNameStatic();
	virtual bool			IsTypeOf(FName TypeName) const { return s_TypeName == TypeName ? true : false; }
	template <class T> bool IsA() const { return IsTypeOf(T::GetTypeNameStatic()); }

	virtual ~FBaseGlyphSequence() {}
	void	   SetID(uint32 ID) { m_PoolID = ID; }
	uint32	   GetID() { return m_PoolID; }
	void	   SetParent(UBaseFont* Font) { m_ParentFont = Font; }
	UBaseFont* GetParent()
	{
		if (m_ParentFont.IsValid())
			return m_ParentFont.Get();
		else
			return nullptr;
	}

	virtual int32 GetGlyphCount() = 0;
	virtual float GetBaseline(float Scale) = 0;
	virtual bool  GetGlyph(int32 GlyphIndex, float& GlyphWidth, float& GlyphHeight, int32& CharIndex) = 0;
	virtual bool  DrawGlyph(
		 int32 Start, int32 Count, FTextMesh& TextMesh, float X, float Y, float LetterSpacing, float Scale) = 0;
	virtual void GetCharToGlyphMap(TArray<int32>& Map) = 0;
	virtual void Clear() = 0;
	virtual void Release();
};

UCLASS()
class UBaseFont : public UObject
{
	GENERATED_BODY()
protected:
	FName m_Name;

public:
	static float DEFAULT_ITALIC_ANGLE;

	UBaseFont() {}
	virtual ~UBaseFont() {}
	FName GetName() { return m_Name; };
	void  SetName(FName Name) { m_Name = Name; }

	virtual void						   SetFormat(const FNTextFormat& format) {}
	virtual TSharedPtr<FBaseGlyphSequence> GetGlyph(const FString& Text) { return nullptr; }
	virtual bool						   HasCharacter(TCHAR ch) { return false; }
	virtual int							   GetLineHeight(int size) { return 0; }
	virtual bool						   ReturnGlyph(TSharedPtr<FBaseGlyphSequence> pGlyphSequence) { return false; }
};