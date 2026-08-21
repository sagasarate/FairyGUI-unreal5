#pragma once

#include "CoreMinimal.h"
#include "Utils/HTML/HTMLObject.h"
#include "HTMLButton.generated.h"

class UGComponent;
class UEventContext;

UCLASS()
class UHTMLButton : public UHTMLObject
{
	GENERATED_BODY()
protected:
	TWeakPtr<SRichTextField> m_Owner;
	UPROPERTY()
	TObjectPtr<UGComponent> m_Button;

public:
	static FString Resource;

public:
	virtual float			GetWidth() override;
	virtual float			GetHeight() override;
	virtual SDisplayObject* GetDisplayObject() override;

	virtual bool Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement) override;
	virtual void SetPosition(float X, float Y) override;
	virtual void Add() override;
	virtual void Remove() override;
	virtual void Release() override;

protected:
	UFUNCTION()
	void OnButtonClick(UEventContext* EventContext);
};