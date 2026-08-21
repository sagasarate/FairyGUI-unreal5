#pragma once

#include "CoreMinimal.h"
#include "Utils/HTML/HTMLObject.h"
#include "HTMLImage.generated.h"

class UGLoader;

UCLASS()
class UHTMLImage : public UHTMLObject
{
	GENERATED_BODY()
protected:
	TWeakPtr<SRichTextField> m_Owner;
	UPROPERTY()
	TObjectPtr<UGLoader> m_Loader;

public:
	virtual float			GetWidth() override;
	virtual float			GetHeight() override;
	virtual SDisplayObject* GetDisplayObject() override;

	virtual bool Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement) override;
	virtual void SetPosition(float X, float Y) override;
	virtual void Add() override;
	virtual void Remove() override;
	virtual void Release() override;

};