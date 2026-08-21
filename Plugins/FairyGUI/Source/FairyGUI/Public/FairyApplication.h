#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine.h"
#include "Framework/Application/IInputProcessor.h"
#include "FairyCommons.h"
#include "Event/EventContext.h"
#include "Tween/TweenManager.h"
#include "UI/UIConfig.h"
#include "FairyApplication.generated.h"

class UUIPackage;
class UNTexture;
class UGObject;
class UGRoot;
class UDragDropManager;

UCLASS(BlueprintType)
class FAIRYGUI_API UFairyApplication : public UObject
{
	GENERATED_BODY()

private:
	struct FTouchInfo
	{
		int32							 UserIndex;
		int32							 PointerIndex;
		bool							 bDown;
		bool							 bToClearCaptors;
		FVector2D						 DownPosition;
		bool							 bClickCancelled;
		int32							 ClickCount;
		TArray<TWeakPtr<SDisplayObject>> DownPath;
		TArray<TWeakObjectPtr<UGObject>> MouseCaptors;
		FPointerEvent					 Event;

		FTouchInfo();
	};

	class FInputProcessor : public IInputProcessor
	{
	public:
		FInputProcessor(UFairyApplication* InApplication);
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
		virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
		virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

		UFairyApplication* Application;
	};

public:
	UFUNCTION(BlueprintPure, Category = "FairyGUI",
		meta = (DisplayName = "Get Application", WorldContext = "WorldContextObject"))
	static UFairyApplication* Get(UObject* WorldContextObject);

	static void Destroy();

	UFairyApplication();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UGRoot* GetUIRoot() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UDragDropManager* GetDragDropManager() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	FVector2D GetTouchPosition(int32 InUserIndex = -1, int32 InPointerIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	int32 GetTouchCount() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UGObject* GetObjectUnderPoint(const FVector2D& ScreenspacePosition);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void CancelClick(int32 InUserIndex = -1, int32 InPointerIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void PlaySound(const FString& URL, float VolumeScale = 1);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsSoundEnabled() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetSoundEnabled(bool InEnabled);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetSoundVolumeScale() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetSoundVolumeScale(float InVolumeScale);

public:
	bool DispatchEvent(
		const FName& EventType, const TSharedRef<SDisplayObject>& Initiator, const FNVariant& Data = FNVariant::Null);
	void BubbleEvent(
		const FName& EventType, const TSharedRef<SDisplayObject>& Initiator, const FNVariant& Data = FNVariant::Null);
	void BroadcastEvent(
		const FName& EventType, const TSharedRef<SDisplayObject>& Initiator, const FNVariant& Data = FNVariant::Null);

	void AddMouseCaptor(int32 InUserIndex, int32 InPointerIndex, UGObject* InTarget);
	void RemoveMouseCaptor(int32 InUserIndex, int32 InPointerIndex, UGObject* InTarget);
	bool HasMouseCaptor(int32 InUserIndex, int32 InPointerIndex);

	bool OnWidgetMouseButtonDown(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);
	bool OnWidgetMouseButtonUp(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);
	bool OnWidgetMouseMove(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);
	bool OnWidgetMouseButtonDoubleClick(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);
	void OnWidgetMouseEnter(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);
	void OnWidgetMouseLeave(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);
	bool OnWidgetMouseWheel(const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent);

	// 键盘 & 焦点
	bool OnWidgetKeyDown(const TSharedRef<SDisplayObject>& DisplayObj, const FKeyEvent& InKeyEvent);
	bool OnWidgetKeyChar(const TSharedRef<SDisplayObject>& DisplayObj, const FCharacterEvent& InCharacterEvent);
	bool OnWidgetKeyUp(const TSharedRef<SDisplayObject>& DisplayObj, const FKeyEvent& InKeyEvent);
	bool OnWidgetFocusReceived(const TSharedRef<SDisplayObject>& DisplayObj, EFocusCause InCause);
	void OnWidgetFocusLost(const TSharedRef<SDisplayObject>& DisplayObj, EFocusCause InCause);

	UGameViewportClient*	   GetViewportClient() const { return ViewportClient; }
	const TSharedPtr<SWidget>& GetViewportWidget() const { return ViewportWidget; }

	void CallAfterSlateTick(FSimpleDelegate Callback);

	template <class UserClass, typename... VarTypes>
	void DelayCall(FTimerHandle& InOutHandle, UserClass* InUserObject,
		typename TMemFunPtrType<false, UserClass, void(VarTypes...)>::Type inTimerMethod, VarTypes...);
	void CancelDelayCall(FTimerHandle& InHandle);

private:
	void OnCreate();
	void OnDestroy();

	void PreviewDownEvent(const FPointerEvent& MouseEvent);
	void PreviewUpEvent(const FPointerEvent& MouseEvent);
	void PreviewMoveEvent(const FPointerEvent& MouseEvent);

	UEventContext* BorrowEventContext();
	void		   ReturnEventContext(UEventContext* Context);

	void InternalBubbleEvent(const FName& EventType, const TArray<UGObject*>& CallChain, const FNVariant& Data);

	FTouchInfo* GetTouchInfo(const FPointerEvent& MouseEvent);
	FTouchInfo* GetTouchInfo(int32 InUserIndex, int32 InPointerIndex);

	void OnSlatePostTick(float DeltaTime);

private:
	UPROPERTY(Transient)
	UGRoot* UIRoot;
	UPROPERTY(Transient)
	UDragDropManager* DragDropManager;
	UPROPERTY(Transient)
	TArray<UEventContext*> EventContextPool;

	TSharedPtr<IInputProcessor> InputProcessor;
	UGameViewportClient*		ViewportClient;
	TSharedPtr<SWidget>			ViewportWidget;
	TIndirectArray<FTouchInfo>	Touches;
	FTouchInfo*					LastTouch;
	bool						bNeedCheckPopups;
	bool						bOnDestroyed = false;
	FDelegateHandle				PostTickDelegateHandle;
	FSimpleMulticastDelegate	PostTickMulticastDelegate;
	bool						bSoundEnabled;
	float						SoundVolumeScale;

	static TMap<uint32, TWeakObjectPtr<UFairyApplication>> Instances;

	protected:
		virtual void BeginDestroy() override;
};

template <class UserClass, typename... VarTypes>
void UFairyApplication::DelayCall(FTimerHandle& InOutHandle, UserClass* InUserObject,
	typename TMemFunPtrType<false, UserClass, void(VarTypes...)>::Type inTimerMethod, VarTypes... Vars)
{
	if (!GetWorld()->GetTimerManager().TimerExists(InOutHandle))
		InOutHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(InUserObject, inTimerMethod, Vars...));
}

inline void UFairyApplication::CancelDelayCall(FTimerHandle& InHandle)
{
	GetWorld()->GetTimerManager().ClearTimer(InHandle);
}