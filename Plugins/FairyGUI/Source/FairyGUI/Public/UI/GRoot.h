#pragma once

#include "GComponent.h"
#include "GRoot.generated.h"

class UGGraph;
class UGWindow;
class UObjectPool;
class SFGUICanvas;

UCLASS(BlueprintType, NotBlueprintable)
class FAIRYGUI_API UGRoot : public UGComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "FairyGUI", meta = (DisplayName = "Get UI Root", WorldContext = "WorldContextObject"))
    static UGRoot* Get(UObject* WorldContextObject);

    static int32 ContentScaleLevel;

    UGRoot();
    virtual ~UGRoot();
    virtual void Dispose() override;

    void ShowWindow(UGWindow* Window);
    void HideWindow(UGWindow* Window);
    void HideWindowImmediately(UGWindow* Window);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void BringToFront(UGWindow* Window);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void ShowModalWait();

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void CloseModalWait();

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void CloseAllExceptModals();

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void CloseAllWindows();

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    UGWindow* GetTopWindow() const;

    UGObject* GetModalWaitingPane();
    UGGraph* GetModalLayer();

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    bool HasModalWindow() const;

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    bool IsModalWaiting() const;

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void ShowPopup(UGObject* Popup, UGObject* AtObject, EPopupDirection Direction);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void TogglePopup(UGObject* Popup, UGObject* AtObject = nullptr, EPopupDirection Direction = EPopupDirection::Auto);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void HidePopup(UGObject* Popup = nullptr);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    bool HasAnyPopup() const;

    FVector2D GetPoupPosition(UGObject* Popup, UGObject* AtObject, EPopupDirection Direction);
    void CheckPopups(SDisplayObject* ClickTarget);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void ShowTooltips(const FText& Text);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
    void ShowTooltipsWin(UGObject* InTooltipWin);

    UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void HideTooltips();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UObjectPool* GetObjectPool();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void ReturnFocus();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void ClearFocus();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UGObject* GetFocusedObject() const;

	TSharedPtr<class SFGUICanvas> GetCanvas() const { return FullScreenCanvas; }

    void AddToViewport();
private:
    void CreateModalLayer();
    void AdjustModalLayer();
    void ClosePopup(UGObject* Popup);

    //void UpdateContentScaleLevel();

	void DoShowTooltipsWin();

	TSharedPtr<SFGUICanvas> FullScreenCanvas;

    UPROPERTY(Transient)
    UGGraph* ModalLayer;
    UPROPERTY(Transient)
    UGObject* ModalWaitPane;
    UPROPERTY(Transient)
    UGObject* TooltipWin;
    UPROPERTY(Transient)
    UGObject* DefaultTooltipWin;
    TArray<TWeakObjectPtr<UGObject>> PopupStack;
    TArray<TWeakObjectPtr<UGObject>> JustClosedPopups;
	FTimerHandle					 ShowTooltipsTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UObjectPool> ObjectPool;

    friend class UFairyApplication;
};
