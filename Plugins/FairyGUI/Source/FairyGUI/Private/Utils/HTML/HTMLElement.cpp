#include "Utils/HTML/HTMLElement.h"
#include "UI/GRoot.h"
#include "Utils/ObjectPool.h"
#include "Utils/HTML/HTMLButton.h"
#include "Utils/HTML/HTMLImage.h"
#include "Utils/HTML/HTMLLink.h"
#include "Utils/HTML/HTMLSelect.h"
#include "Utils/HTML/HTMLInput.h"
#include "Widgets/SRichTextField.h"
#include "Widgets/Font/BaseFont.h"
#include "UI/UIConfig.h"

CIDStorage<FHTMLElement> FHTMLElement::m_Pool;

void FHTMLElement::Clear()
{
	m_Type = EHTMLElementType::Text;
	m_Name.Empty();
	m_Text.Empty();
	m_CharIndex = 0;
	m_Space = 0;
	m_Attributes.Empty();
	ReleaseHTMLObject();
}

UHTMLObject* FHTMLElement::CreateHTMLObject(TSharedPtr<SRichTextField> Owner, UObject* WorldContextObject)
{
	auto ObjectPool = UGRoot::Get(WorldContextObject)->GetObjectPool();
	if (m_HTMLObject.IsValid())
	{
		m_HTMLObject->Release();
		ObjectPool->Return(m_HTMLObject.Get());
		m_HTMLObject = nullptr;
	}
	switch (m_Type)
	{
		case EHTMLElementType::Link:
			m_HTMLObject = ObjectPool->Borrow<UHTMLLink>(true);
			break;
		case EHTMLElementType::Image:
			m_HTMLObject = ObjectPool->Borrow<UHTMLImage>(true);
			break;
		case EHTMLElementType::Input:
		{
			auto Type = m_Attributes.Get(TEXT("type"));
			Type.ToLowerInline();
			if (Type == TEXT("button") || Type == TEXT("submit"))
			{
				if (!UHTMLButton::Resource.IsEmpty())
					m_HTMLObject = ObjectPool->Borrow<UHTMLButton>(true);
			}
			else
			{
				m_HTMLObject = ObjectPool->Borrow<UHTMLInput>(true);
			}
		}
		break;
		case EHTMLElementType::Select:
			if (!UHTMLSelect::Resource.IsEmpty())
				m_HTMLObject = ObjectPool->Borrow<UHTMLSelect>(true);
			break;
	}
	if (m_HTMLObject.IsValid())
	{
		if (!m_HTMLObject->Create(Owner, this))
			ReleaseHTMLObject();
	}
	return m_HTMLObject.Get();
}

void FHTMLElement::ReleaseHTMLObject()
{
	if (m_HTMLObject.IsValid())
	{
		m_HTMLObject->Release();
		auto ObjectPool = UGRoot::Get(m_HTMLObject->GetWorld())->GetObjectPool();
		if (ObjectPool)
			ObjectPool->Return(m_HTMLObject.Get());
		m_HTMLObject = nullptr;
	}
}

FHTMLElement* FHTMLElement::Borrow()
{
	if (m_Pool.GetBufferSize() == 0)
		m_Pool.Create(256, 256, FUIConfig::Config.HTMLElementPoolGrowLimit);
	FHTMLElement* pInfo = m_Pool.NewObject();
	if (pInfo == nullptr)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("FHTMLElement pool is full(%u/%u)!"), m_Pool.GetObjectCount(),
			m_Pool.GetBufferSize());
	}
	return pInfo;
}

void FHTMLElement::Return(FHTMLElement* pValue)
{
	pValue->Clear();
	m_Pool.DeleteObject(pValue->m_PoolID);
	pValue->m_PoolID = 0;
}

void FHTMLElement::Return(TArray<FHTMLElement*>& Values)
{
	for (auto& Info : Values)
		Return(Info);
	Values.Empty();
}