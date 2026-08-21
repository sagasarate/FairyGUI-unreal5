#pragma once

#include "CoreMinimal.h"
#include "Utils/HTML/HTMLObject.h"
#include "Slate.h"
#include "HTMLLink.generated.h"

class SSelectionShape;

UCLASS()
class UHTMLLink : public UHTMLObject
{
	GENERATED_BODY()
protected:
	TWeakPtr<SRichTextField>	m_Owner;
	TSharedPtr<SSelectionShape> m_Sharp;

	FDelegateHandle m_OnClickHandle;
	FDelegateHandle m_OnRolloverHandle;
	FDelegateHandle m_OnRolloutHandle;

public:
	virtual float			GetWidth() override;
	virtual float			GetHeight() override;
	virtual SDisplayObject* GetDisplayObject() override;

	virtual bool Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement) override;
	virtual void SetPosition(float X, float Y) override;
	virtual void Add() override;
	virtual void Remove() override;
	virtual void Release() override;

	void SetArea(int32 StartCharIndex, int32 EndCharIndex);
	void SetSize(float Width, float Height);
protected:
	void OnClick(const FPointerEvent& MouseEvent);
	void OnRollover(const FPointerEvent& MouseEvent);
	void OnRollout(const FPointerEvent& MouseEvent);
};