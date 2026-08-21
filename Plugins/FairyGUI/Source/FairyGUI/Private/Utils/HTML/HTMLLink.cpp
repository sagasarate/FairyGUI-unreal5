#include "Utils/HTML/HTMLLink.h"
#include "UI/UIPackage.h"
#include "UI/GRoot.h"
#include "Widgets/SRichTextField.h"
#include "Widgets/SSelectionShape.h"
#include "Utils/ObjectPool.h"
#include "Event/EventTypes.h"

float UHTMLLink::GetWidth()
{
	return m_Sharp->GetWidth();
}
float UHTMLLink::GetHeight()
{
	return m_Sharp->GetHeight();
}
SDisplayObject* UHTMLLink::GetDisplayObject()
{
	return m_Sharp.Get();
}

bool UHTMLLink::Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement)
{
	if (!Owner.IsValid() || !Owner->GObject.IsValid() || !pElement)
		return false;
	m_Owner = Owner;
	m_pElement = pElement;
	m_Sharp = SSelectionShape::Borrow();
	m_Sharp->SetMouseCursor(EMouseCursor::Hand);
	m_Sharp->SetInteractable(true);
	m_Sharp->EnableHitTest(true);
	return m_Sharp != nullptr;
}
void UHTMLLink::SetPosition(float X, float Y)
{
	if (m_Sharp)
		m_Sharp->SetPosition(FVector2D(X, Y));
}
void UHTMLLink::Add()
{
	auto Owner = m_Owner.Pin();
	if (Owner && m_Sharp)
	{
		Owner->AddChild(m_Sharp.ToSharedRef(), EDisplayObjChildLayer::Back);
		if (m_OnClickHandle.IsValid())
		{
			m_Sharp->OnClick.Remove(m_OnClickHandle);
			m_OnClickHandle.Reset();
		}
		if (m_OnRolloverHandle.IsValid())
		{
			m_Sharp->OnClick.Remove(m_OnRolloverHandle);
			m_OnRolloverHandle.Reset();
		}
		if (m_OnRolloutHandle.IsValid())
		{
			m_Sharp->OnClick.Remove(m_OnRolloutHandle);
			m_OnRolloutHandle.Reset();
		}
		m_OnClickHandle = m_Sharp->OnClick.AddUObject(this, &UHTMLLink::OnClick);
		m_OnRolloverHandle = m_Sharp->OnRollover.AddUObject(this, &UHTMLLink::OnRollover);
		m_OnRolloutHandle = m_Sharp->OnRollout.AddUObject(this, &UHTMLLink::OnRollout);
	}
}
void UHTMLLink::Remove()
{
	auto Owner = m_Owner.Pin();
	if (Owner && m_Sharp)
	{
		Owner->RemoveChild(m_Sharp.ToSharedRef());
		if (m_OnClickHandle.IsValid())
		{
			m_Sharp->OnClick.Remove(m_OnClickHandle);
			m_OnClickHandle.Reset();
		}
		if (m_OnRolloverHandle.IsValid())
		{
			m_Sharp->OnClick.Remove(m_OnRolloverHandle);
			m_OnRolloverHandle.Reset();
		}
		if (m_OnRolloutHandle.IsValid())
		{
			m_Sharp->OnClick.Remove(m_OnRolloutHandle);
			m_OnRolloutHandle.Reset();
		}
	}
}
void UHTMLLink::Release()
{
	Remove();
	if (m_Sharp)
		SSelectionShape::Return(m_Sharp);
	m_Sharp = nullptr;
	m_Owner = nullptr;
	m_pElement = nullptr;
}

void UHTMLLink::SetArea(int32 StartCharIndex, int32 EndCharIndex)
{
	if (!m_Sharp)
		return;
	auto Owner = m_Owner.Pin();
	if (!Owner)
		return;
	if (StartCharIndex > EndCharIndex)
	{
		int32 tmp = StartCharIndex;
		StartCharIndex = EndCharIndex;
		EndCharIndex = tmp;
	}
	TArray<FBox2D> Rects;
	Owner->GetLinesShape(StartCharIndex, EndCharIndex, true, Rects);
	m_Sharp->SetRects(Rects, Owner->HTMLParseOptions.LinkBgColor);
}
void UHTMLLink::SetSize(float Width, float Height)
{
	if (m_Sharp)
		m_Sharp->SetSize(FVector2D(Width, Height));
}

void UHTMLLink::OnClick(const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		auto Owner = m_Owner.Pin();
		if (Owner && Owner->GObject.IsValid() && m_pElement)
		{
			auto& href = m_pElement->GetAttributes().Get(TEXT("href"));
			Owner->GObject->DispatchEvent(FUIEvents::HtmlLinkClick, FNVariant(href));
		}
	}
}
void UHTMLLink::OnRollover(const FPointerEvent& MouseEvent)
{
	auto Owner = m_Owner.Pin();
	if (Owner && Owner->GObject.IsValid() && m_pElement)
	{
		auto& href = m_pElement->GetAttributes().Get(TEXT("href"));
		Owner->GObject->DispatchEvent(FUIEvents::HtmlLinkRollOver, FNVariant(href));
	}
}
void UHTMLLink::OnRollout(const FPointerEvent& MouseEvent)
{
	auto Owner = m_Owner.Pin();
	if (Owner && Owner->GObject.IsValid() && m_pElement)
	{
		auto& href = m_pElement->GetAttributes().Get(TEXT("href"));
		Owner->GObject->DispatchEvent(FUIEvents::HtmlLinkRollOut, FNVariant(href));
	}
}