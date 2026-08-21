#include "Utils/HTML/HTMLButton.h"
#include "UI/UIPackage.h"
#include "Widgets/SRichTextField.h"
#include "UI/GComponent.h"
#include "Event/EventContext.h"
#include "Event/EventTypes.h"
#include "UI/GRoot.h"
#include "Utils/ObjectPool.h"

FString UHTMLButton::Resource;

float UHTMLButton::GetWidth()
{
	if (m_Button)
		return m_Button->GetWidth();
	return 0;
}
float UHTMLButton::GetHeight()
{
	if (m_Button)
		return m_Button->GetHeight();
	return 0;
}
SDisplayObject* UHTMLButton::GetDisplayObject()
{
	if (m_Button)
		return &m_Button->GetDisplayObject().Get();
	return nullptr;
}

bool UHTMLButton::Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement)
{
	if (!Owner.IsValid() || !Owner->GObject.IsValid() || !pElement)
		return false;
	m_Owner = Owner;
	m_pElement = pElement;

	m_Button = Cast<UGComponent>(UGRoot::Get(GetWorld())->GetObjectPool()->Borrow(Resource, false));
	if (m_Button)
		m_Button->Name = TEXT("HtmlButton");
	return m_Button != nullptr;
}
void UHTMLButton::SetPosition(float X, float Y)
{
	if (m_Button)
		m_Button->SetXY(X, Y);
}
void UHTMLButton::Add()
{
	auto Owner = m_Owner.Pin();
	if (m_Button && Owner)
	{
		Owner->AddChild(m_Button->GetDisplayObject());
		m_Button->OnClick.AddUniqueDynamic(this, &UHTMLButton::OnButtonClick);
	}
}
void UHTMLButton::Remove()
{
	auto Owner = m_Owner.Pin();
	if (m_Button && Owner)
	{
		Owner->RemoveChild(m_Button->GetDisplayObject());
		m_Button->OnClick.RemoveDynamic(this, &UHTMLButton::OnButtonClick);
	}
}
void UHTMLButton::Release()
{
	Remove();
	if (m_Button)
		UGRoot::Get(GetWorld())->GetObjectPool()->Return(m_Button);
	m_Button = nullptr;
	m_Owner = nullptr;
	m_pElement = nullptr;
}
void UHTMLButton::OnButtonClick(UEventContext* EventContext)
{
	auto Owner = m_Owner.Pin();
	if (Owner && Owner->GObject.IsValid())
		Owner->GObject->DispatchEvent(FUIEvents::HtmlButtonClick, FNVariant(this));
}