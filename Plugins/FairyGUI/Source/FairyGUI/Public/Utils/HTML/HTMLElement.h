#pragma once

#include "Slate.h"
#include "Widgets/Font/NTextFormat.h"
#include "Utils/HTML/XMLAttributes.h"
#include "Utils/HTML/HTMLObject.h"
#include "Utils/IDStorage.h"
#include "FairyCommons.h"

class SRichTextField;
class FBaseGlypSequence;
class UBaseFont;

enum class EHTMLElementType
{
	Text,
	Link,
	Image,
	Input,
	Select,
	Object,

	// internal
	LinkEnd,
	NewLine
};

class FAIRYGUI_API FHTMLElement
{
protected:
	uint32			 m_PoolID = 0;
	EHTMLElementType m_Type = EHTMLElementType::Text;
	FString			 m_Name;
	FString			 m_Text;
	FNTextFormat	 m_Format;
	int32			 m_CharIndex = 0;

	int32						m_Space = 0;
	FVector2D					m_Position;
	FXMLAttributes				m_Attributes;
	TWeakObjectPtr<UHTMLObject> m_HTMLObject = nullptr;

	static CIDStorage<FHTMLElement> m_Pool;

public:
	void Clear();
	void SetID(uint32 ID) { m_PoolID = ID; }

	void			 SetType(EHTMLElementType Type) { m_Type = Type; }
	EHTMLElementType GetType() const { return m_Type; }

	const FString&	GetName() const { return m_Name; }
	void			SetName(const FString& Name) { m_Name = Name; }
	FString&		GetText() { return m_Text; }
	const FString&	GetText() const { return m_Text; }
	FNTextFormat&	GetFormat() { return m_Format; }
	FXMLAttributes& GetAttributes() { return m_Attributes; }

	void  SetSpace(int32 Space) { m_Space = Space; }
	int32 GetSpace() { return m_Space; }

	const FVector2D& GetPosition() { return m_Position; }

	void  SetCharIndex(int32 Index) { m_CharIndex = Index; }
	int32 GetCharIndex() const { return m_CharIndex; }

	bool IsEntity() const
	{

		return m_Type == EHTMLElementType::Image || m_Type == EHTMLElementType::Select
			|| m_Type == EHTMLElementType::Input || m_Type == EHTMLElementType::Object;
	}
	UHTMLObject* CreateHTMLObject(TSharedPtr<SRichTextField> Owner, UObject* WorldContextObject);
	void ReleaseHTMLObject();
	UHTMLObject* GetHTMLObject()
	{
		if (m_HTMLObject.IsValid())
			return m_HTMLObject.Get();
		return nullptr;
	};

	static FHTMLElement* Borrow();
	static void			 Return(FHTMLElement* pValue);
	static void			 Return(TArray<FHTMLElement*>& Values);
};
