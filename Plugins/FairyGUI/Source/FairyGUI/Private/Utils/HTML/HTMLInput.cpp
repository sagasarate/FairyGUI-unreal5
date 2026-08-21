#include "Utils/HTML/HTMLInput.h"
#include "UI/UIPackage.h"
#include "UI/GRoot.h"
#include "Widgets/SRichTextField.h"
#include "UI/GTextInput.h"
#include "Utils/ObjectPool.h"
#include "Widgets/Font/FontManager.h"
#include "Widgets/Font/DynamicFont.h"
#include "Widgets/STextField.h"

int32  UHTMLInput::DefaultBorderSize = 2;
FColor UHTMLInput::DefaultBorderColor = FColor(0xFFA9A9A9);
FColor UHTMLInput::DefaultBackgroundColor = FColor::Transparent;

float UHTMLInput::GetWidth()
{
	if (m_Input)
		return m_Input->GetWidth();
	return 0;
}
float UHTMLInput::GetHeight()
{
	if (m_Input)
		return m_Input->GetHeight();
	return 0;
}
SDisplayObject* UHTMLInput::GetDisplayObject()
{
	if (m_Input)
		return &m_Input->GetDisplayObject().Get();
	return nullptr;
}

bool UHTMLInput::Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement)
{
	if (!Owner.IsValid() || !Owner->GObject.IsValid() || !pElement)
		return false;
	m_Owner = Owner;
	m_pElement = pElement;

	m_Input = Cast<UGTextInput>(UGRoot::Get(GetWorld())->GetObjectPool()->Borrow(EObjectType::InputText, false));
	if (!m_Input)
		return false;

	m_Input->Name = TEXT("HtmlInput");

	auto Type = m_pElement->GetAttributes().Get(TEXT("type"));
	Type.ToLowerInline();

	m_bHidden = Type == TEXT("hidden");
	if (!m_bHidden)
	{
		int32  Width = m_pElement->GetAttributes().GetInt(TEXT("width"), 0);
		int32  Height = m_pElement->GetAttributes().GetInt(TEXT("height"), 0);
		int32  BorderSize = m_pElement->GetAttributes().GetInt(TEXT("border"), DefaultBorderSize);
		FColor BorderColor = m_pElement->GetAttributes().GetColor(TEXT("border-color"), DefaultBorderColor);
		FColor BackgroundColor = m_pElement->GetAttributes().GetColor(TEXT("background-color"), DefaultBackgroundColor);

		if (Width == 0 && Owner->GObject.IsValid())
		{
			Width = m_pElement->GetSpace();
			if (Width > Owner->GObject->GetWidth() / 2 || Width < 100)
				Width = (int32)Owner->GObject->GetWidth() / 2;
		}
		if (Height == 0)
		{
			FName CurFontName = G_DEFAULT_FONT_NAME;
			if (!m_pElement->GetFormat().Face.IsNone())
				CurFontName = m_pElement->GetFormat().Face;
			UBaseFont* pCurFont = UUIPackage::GetFontManager()->GetFont(CurFontName);
			pCurFont->SetFormat(m_pElement->GetFormat());
			Height = pCurFont->GetLineHeight(m_pElement->GetFormat().Size) + STextField::GUTTER_Y * 2;
		}
		else
		{
			Height = m_pElement->GetFormat().Size * UDynamicFont::LINE_HEIGHT_FACTOR + STextField::GUTTER_Y * 2;
		}

		m_Input->SetTextFormat(m_pElement->GetFormat());
		m_Input->SetPassword(Type == TEXT("password"));
		m_Input->SetMaxLength(m_pElement->GetAttributes().GetInt(TEXT("maxlength"), INT32_MAX));
		m_Input->SetBorder(BorderSize);
		m_Input->SetBorderColor(BorderColor);
		m_Input->SetBackgroundColor(BackgroundColor);
		m_Input->SetSize(Width, Height);
	}
	m_Input->SetText(FText::FromString(m_pElement->GetAttributes().Get(TEXT("value"))));
	return true;
}
void UHTMLInput::SetPosition(float X, float Y)
{
	if (m_Input)
		m_Input->SetXY(X, Y);
}
void UHTMLInput::Add()
{
	auto Owner = m_Owner.Pin();
	if (m_Input && Owner)
	{
		Owner->AddChild(m_Input->GetDisplayObject());
	}
}
void UHTMLInput::Remove()
{
	auto Owner = m_Owner.Pin();
	if (m_Input && Owner)
	{
		Owner->RemoveChild(m_Input->GetDisplayObject());
	}
}
void UHTMLInput::Release()
{
	Remove();
	if (m_Input)
		UGRoot::Get(GetWorld())->GetObjectPool()->Return(m_Input);
	m_Input = nullptr;
	m_Owner = nullptr;
	m_pElement = nullptr;
}
