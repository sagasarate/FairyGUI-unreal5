#pragma once

#include "CoreMinimal.h"
#include "HTMLObject.generated.h"

class FHTMLElement;
class SRichTextField;
class SDisplayObject;

UCLASS()
class UHTMLObject : public UObject
{
	GENERATED_BODY()
protected:
	FHTMLElement* m_pElement;

public:
	virtual ~UHTMLObject() {}
	FHTMLElement* GetHTMLElement() { return m_pElement; }

	virtual float			GetWidth() { return 0; }
	virtual float			GetHeight() { return 0; }
	virtual SDisplayObject* GetDisplayObject() { return nullptr; }

	virtual bool Create(TSharedPtr<SRichTextField> Owner, FHTMLElement* pElement) { return false; }
	virtual void SetPosition(float X, float Y) {}
	virtual void Add() {}
	virtual void Remove() {}
	virtual void Release() {}
};