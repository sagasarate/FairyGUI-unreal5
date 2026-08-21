#pragma once

#include "GObject.h"
#include "UI/GRichTextField.h"
#include "Widgets/SInputTextField.h"
#include "Utils/UBBParser.h"
#include "GTextInput.generated.h"

UCLASS(BlueprintType)
class FAIRYGUI_API UGTextInput : public UGRichTextField
{
	GENERATED_BODY()
protected:
	TSharedPtr<class SInputTextField> InputContent;

public:
	UGTextInput();
	virtual ~UGTextInput();
	virtual void Dispose() override;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnSubmit;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnChanged;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnCancel;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnGetFocus;

	UPROPERTY(BlueprintAssignable, Category = "FairyGUI|Event")
	FGUIEventDynMDelegate OnLostFocus;

	virtual FText GetText() const override;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	const FString& GetInputText();
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetInputText(const FString& InText);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetPrompt(const FText& InPrompt);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetPassword(bool bInPassword);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetKeyboardType(int32 InKeyboardType);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetMaxLength(int32 InMaxLength);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetRestrict(const FString& InRestrict);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	int32 GetMaxLength();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetBorder(int32 Border);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	int32 GetBorder();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetBorderColor(const FColor& color);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	int32 GetCorner();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetCorner(int32 Corner);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	FColor GetBorderColor();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetBackgroundColor(const FColor& color);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	FColor GetBackgroundColor();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsEditable();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetEditable(bool bEditable);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsIMEEnabled();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void EnableIME(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsMouseWheelEnabled();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void EnableMouseWheel(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void ClearSelection();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void ReplaceSelection(const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	FString GetSelection();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetSelection(int32 Start, int32 Length);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsWantReturn() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetWantReturn(bool bWant);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsWantTab() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetWantTab(bool bWant);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsAutoSubmitOnLostFocus() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetAutoSubmitOnLostFocus(bool bEnable);

protected:
	virtual void CreateDisplayObject() override;
	virtual void SetupBeforeAdd(FByteBuffer* Buffer, int32 BeginPos) override;
	virtual void SetupAfterAdd(FByteBuffer* Buffer, int32 BeginPos) override;
};
