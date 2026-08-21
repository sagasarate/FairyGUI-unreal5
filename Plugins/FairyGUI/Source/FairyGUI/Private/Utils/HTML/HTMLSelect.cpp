#include "Utils/HTML/HTMLSelect.h"
#include "UI/UIPackage.h"
#include "Widgets/SRichTextField.h"
#include "UI/GComboBox.h"
#include "Event/EventContext.h"
#include "Event/EventTypes.h"
#include "UI/GRoot.h"
#include "Utils/ObjectPool.h"

FString UHTMLSelect::Resource;

float UHTMLSelect::GetWidth()
{
	if (m_ComboBox)
		return m_ComboBox->GetWidth();
	return 0;
}
float UHTMLSelect::GetHeight()
{
	if (m_ComboBox)
		return m_ComboBox->GetHeight();
	return 0;
}
SDisplayObject* UHTMLSelect::GetDisplayObject()
{
	if (m_ComboBox)
		return &m_ComboBox->GetDisplayObject().Get();
	return nullptr;
}

bool UHTMLSelect::Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement)
{
	if (!Owner.IsValid() || !Owner->GObject.IsValid() || !pElement)
		return false;
	m_Owner = Owner;
	m_pElement = pElement;

	m_ComboBox = Cast<UGComboBox>(UGRoot::Get(GetWorld())->GetObjectPool()->Borrow(Resource, false));
	if (!m_ComboBox)
		return false;
	m_ComboBox->Name = TEXT("HtmlSelect");
	int Width = m_pElement->GetAttributes().GetInt("width", m_ComboBox->InitSize.X);
	int Height = m_pElement->GetAttributes().GetInt("height", m_ComboBox->InitSize.Y);
	m_ComboBox->SetSize(Width, Height);
	auto&			Items = m_pElement->GetAttributes().Get("items");
	TArray<FString> ItemList;
	Items.ParseIntoArrayWS(ItemList, TEXT(","));
	for (auto& Item : ItemList)
	{
		m_ComboBox->Items.Add(FText::AsCultureInvariant(Item));
	}
	auto& Values = m_pElement->GetAttributes().Get("values");
	Values.ParseIntoArrayWS(m_ComboBox->Values, TEXT(","));

	m_ComboBox->SetValue(m_pElement->GetAttributes().Get("value"));
	return true;
}
void UHTMLSelect::SetPosition(float X, float Y)
{
	if (m_ComboBox)
		m_ComboBox->SetXY(X, Y);
}
void UHTMLSelect::Add()
{
	auto Owner = m_Owner.Pin();
	if (m_ComboBox && Owner)
	{
		Owner->AddChild(m_ComboBox->GetDisplayObject());
		m_ComboBox->OnChanged.AddUniqueDynamic(this, &UHTMLSelect::OnChange);
	}
}
void UHTMLSelect::Remove()
{
	auto Owner = m_Owner.Pin();
	if (m_ComboBox && Owner)
	{
		Owner->RemoveChild(m_ComboBox->GetDisplayObject());
		m_ComboBox->OnChanged.RemoveDynamic(this, &UHTMLSelect::OnChange);
	}
}
void UHTMLSelect::Release()
{
	Remove();
	if (m_ComboBox)
		UGRoot::Get(GetWorld())->GetObjectPool()->Return(m_ComboBox);
	m_ComboBox = nullptr;
	m_Owner = nullptr;
	m_pElement = nullptr;
}
void UHTMLSelect::OnChange(UEventContext* EventContext)
{
	auto Owner = m_Owner.Pin();
	if (Owner && Owner->GObject.IsValid())
		Owner->GObject->DispatchEvent(FUIEvents::HtmlSelectChanged, FNVariant(this));
}