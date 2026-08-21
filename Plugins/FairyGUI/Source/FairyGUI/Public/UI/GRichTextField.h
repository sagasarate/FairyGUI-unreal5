#pragma once

#include "GTextField.h"
#include "Widgets/SRichTextField.h"
#include "GRichTextField.generated.h"

UCLASS(BlueprintType)
class FAIRYGUI_API UGRichTextField : public UGTextField
{
	GENERATED_BODY()

public:
	UGRichTextField();
	virtual ~UGRichTextField();

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnHtmlLinkClick;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnHtmlLinkRollOver;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnHtmlLinkRollOut;

	TMap<uint32, SRichTextField::Emoji>& GetEmojis()
	{
		return StaticCastSharedPtr<SRichTextField>(DisplayObject)->GetEmojis();
	}

protected:
	virtual void CreateDisplayObject() override;
};