#pragma once

#include "GObject.h"
#include "Widgets/Font/NTextFormat.h"
#include "Widgets/STextField.h"
#include "GTextField.generated.h"

UCLASS(BlueprintType)
class FAIRYGUI_API UGTextField : public UGObject
{
	GENERATED_BODY()

public:
	UGTextField();
	virtual ~UGTextField();
	virtual void Dispose() override;

	virtual FText GetText() const override;
	virtual void  SetText(const FText& InText) override;

	virtual const FColor& GetColor() const override;
	virtual void		  SetColor(const FColor& InColor) override;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsUBBEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetUBBEnabled(bool InEnabled);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	EAutoSizeType GetAutoSize() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	virtual void SetAutoSize(EAutoSizeType InAutoSize);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	EAlignType GetAlign() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetAlign(EAlignType Align);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	EVerticalAlignType GetVerticalAlign() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetVerticalAlign(EVerticalAlignType Align);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	virtual bool IsSingleLine() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	virtual void SetSingleLine(bool InSingleLine);

	UFUNCTION(BlueprintPure, Category = "FairyGUI")
	FNTextFormat& GetTextFormat();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetTextFormat(const FNTextFormat& InTextFormat);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	virtual FVector2D GetTextSize();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UGTextField* SetVar(const FString& VarKey, const FString& VarValue);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void FlushVars();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void EnableWrapBreakByWord(bool beEnable);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsWrapBreakByWord();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void RebuildText();

	virtual FNVariant GetProp(EObjectPropID PropID) const override;
	virtual void	  SetProp(EObjectPropID PropID, const FNVariant& InValue) override;

	TOptional<TMap<FString, FString>> TemplateVars;

protected:
	virtual void CreateDisplayObject() override;
	virtual void SetupBeforeAdd(FByteBuffer* Buffer, int32 BeginPos) override;
	virtual void SetupAfterAdd(FByteBuffer* Buffer, int32 BeginPos) override;

	void	UpdateSize();
	FString ParseTemplate(const FString& Template);
	void	OnLanguageChanged();

	FText Text;

	TSharedPtr<class STextField> Content;
};