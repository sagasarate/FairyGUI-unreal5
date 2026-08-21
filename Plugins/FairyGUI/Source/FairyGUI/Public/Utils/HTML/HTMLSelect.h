#pragma once

#include "CoreMinimal.h"
#include "Utils/HTML/HTMLObject.h"
#include "HTMLSelect.generated.h"

class UGComboBox;
class UEventContext;

UCLASS()
class UHTMLSelect : public UHTMLObject
{
	GENERATED_BODY()
protected:
	TWeakPtr<SRichTextField> m_Owner;
	UPROPERTY()
	TObjectPtr<UGComboBox> m_ComboBox;

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
	void OnChange(UEventContext* EventContext);
};