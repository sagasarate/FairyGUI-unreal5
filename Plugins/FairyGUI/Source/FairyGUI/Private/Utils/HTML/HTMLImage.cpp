#include "Utils/HTML/HTMLImage.h"
#include "UI/UIPackage.h"
#include "UI/GRoot.h"
#include "Widgets/SRichTextField.h"
#include "UI/Gloader.h"
#include "Utils/ObjectPool.h"

float UHTMLImage::GetWidth()
{
	if (m_Loader)
		return m_Loader->GetWidth();
	return 0;
}
float UHTMLImage::GetHeight()
{
	if (m_Loader)
		return m_Loader->GetHeight();
	return 0;
}
SDisplayObject* UHTMLImage::GetDisplayObject()
{
	if (m_Loader)
		return &m_Loader->GetDisplayObject().Get();
	return nullptr;
}

bool UHTMLImage::Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement)
{
	if (!Owner.IsValid() || !Owner->GObject.IsValid() || !pElement)
		return false;
	m_Owner = Owner;
	m_pElement = pElement;

	m_Loader = Cast<UGLoader>(UGRoot::Get(GetWorld())->GetObjectPool()->Borrow(EObjectType::Loader, false));
	if (!m_Loader)
		return false;

	m_Loader->Name = TEXT("HtmlImage");


	int32 SourceWidth = 0;
	int32 SourceHeight = 0;

	auto& Src = pElement->GetAttributes().Get(TEXT("src"));
	if (!Src.IsEmpty())
	{
		auto pi = UUIPackage::GetItemByURL(Src);
		if (pi)
		{
			SourceWidth = pi->Size.X;
			SourceHeight = pi->Size.Y;
		}
	}

	m_Loader->SetURL(Src);

	int32 Width = pElement->GetAttributes().GetInt(TEXT("width"), SourceWidth);
	int32 Height = pElement->GetAttributes().GetInt(TEXT("height"), SourceHeight);

	if (Width == 0)
		Width = 5;
	if (Height == 0)
		Height = 10;
	m_Loader->SetSize(Width, Height);
	return true;
}
void UHTMLImage::SetPosition(float X, float Y)
{
	if (m_Loader)
		m_Loader->SetXY(X, Y);
}
void UHTMLImage::Add()
{
	auto Owner = m_Owner.Pin();
	if (m_Loader && Owner)
	{
		Owner->AddChild(m_Loader->GetDisplayObject());
	}
}
void UHTMLImage::Remove()
{
	auto Owner = m_Owner.Pin();
	if (m_Loader && Owner)
	{
		Owner->RemoveChild(m_Loader->GetDisplayObject());
	}
}
void UHTMLImage::Release()
{
	Remove();
	if (m_Loader)
		UGRoot::Get(GetWorld())->GetObjectPool()->Return(m_Loader);
	m_Loader = nullptr;
	m_Owner = nullptr;
	m_pElement = nullptr;
}
