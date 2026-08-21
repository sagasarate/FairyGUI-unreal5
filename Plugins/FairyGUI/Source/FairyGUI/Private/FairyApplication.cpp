#include "FairyApplication.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SGameLayerManager.h"
#include "UIPackageAsset.h"
#include "UI/GRoot.h"
#include "UI/UIPackage.h"
#include "UI/UIObjectFactory.h"
#include "UI/PackageItem.h"
#include "UI/GWindow.h"
#include "UI/PopupMenu.h"
#include "UI/DragDropManager.h"
#include "Tween/TweenManager.h"
#include "Widgets/NTexture.h"
#include "Widgets/Font/FontManager.h"
#include "Utils/ByteBuffer.h"

TMap<uint32, TWeakObjectPtr<UFairyApplication>> UFairyApplication::Instances;

struct FMyScopedSwitchWorldHack
{
	FMyScopedSwitchWorldHack(UWorld* World)
	{
		PrevWorld = GWorld;
		GWorld = World;
	}

	~FMyScopedSwitchWorldHack() { GWorld = PrevWorld; }
	UWorld* PrevWorld;
};

UFairyApplication::FTouchInfo::FTouchInfo()
	: UserIndex(0), PointerIndex(0), bDown(false), DownPosition(0, 0), bClickCancelled(false), ClickCount(0)
{
}

UFairyApplication::FInputProcessor::FInputProcessor(UFairyApplication* InApplication)
{
	Application = InApplication;
}

void UFairyApplication::FInputProcessor::Tick(
	const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}

bool UFairyApplication::FInputProcessor::HandleMouseButtonDownEvent(
	FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	Application->PreviewDownEvent(MouseEvent);

	return false;
}

bool UFairyApplication::FInputProcessor::HandleMouseButtonUpEvent(
	FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	Application->PreviewUpEvent(MouseEvent);
	return false;
}

bool UFairyApplication::FInputProcessor::HandleMouseMoveEvent(
	FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	Application->PreviewMoveEvent(MouseEvent);
	return false;
}

UFairyApplication* UFairyApplication::Get(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject->GetWorld();
	verifyf(World != nullptr, TEXT("Null World?"));
	verifyf(World->IsGameWorld(), TEXT("Not a Game World?"));

	UGameInstance*					   GI = World->GetGameInstance();
	TWeakObjectPtr<UFairyApplication>* InstancePtr = Instances.Find(GI->GetUniqueID());
	UFairyApplication*				   Instance = InstancePtr ? InstancePtr->Get() : nullptr;
	if (Instance == nullptr)
	{
		// Outer = GameInstance，Outer 链自然落入 World，无需 AddToRoot()
		Instance = NewObject<UFairyApplication>(GI);
		Instances.Add(GI->GetUniqueID(), Instance);
		Instance->OnCreate();
	}
	return Instance;
}

void UFairyApplication::Destroy()
{
	UUIObjectFactory::PackageItemExtensions.Reset();
	UUIObjectFactory::LoaderExtension = nullptr;

	// 先 Dispose UI 树 —— 触发 Slate widget 析构，归还 Glyph 到字体池
	// 必须在 UUIPackageStatic::Destroy() 之前执行，确保字体 UObject 仍存活
	for (auto& it : Instances)
	{
		if (it.Value.IsValid())
			it.Value->OnDestroy();
	}
	Instances.Reset();

	UNTexture::DestroyWhiteTexture();
	UUIPackageStatic::Destroy();
	FUIConfig::Config = FUIConfig(); // Reset Configuration to default values
}

UFairyApplication::UFairyApplication()
	: bSoundEnabled(true), SoundVolumeScale(1)
{
	LastTouch = new FTouchInfo();
	Touches.Add(LastTouch);
}

void UFairyApplication::OnCreate()
{
	ViewportClient = GetWorld()->GetGameViewport();
	if (ViewportClient == nullptr)
		return;

	ViewportWidget = ViewportClient->GetGameViewportWidget();

	DragDropManager = NewObject<UDragDropManager>(this);
	DragDropManager->CreateAgent();

	PostTickDelegateHandle =
		FSlateApplication::Get().OnPostTick().AddUObject(this, &UFairyApplication::OnSlatePostTick);

	InputProcessor = MakeShareable(new FInputProcessor(this));
	FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
}

void UFairyApplication::OnDestroy()
{
	// 防重入：静态 Destroy() 已调用过，后续 BeginDestroy 不再重复执行
	if (bOnDestroyed)
	{
		return;
	}
	bOnDestroyed = true;

	// 1. 递归 Dispose UI 树（触发 Slate widget 析构 -> 归还静态池）
	//    必须在 FontManager/TweenManager 清理前执行
	if (UIRoot)
	{
		UIRoot->Dispose();
		UIRoot = nullptr;
	}

	// 2. 清理 Tween 单例
	FTweenManager::Singleton.Reset();

	// 3. 注销 Slate 回调
	if (InputProcessor.IsValid())
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);

	if (PostTickDelegateHandle.IsValid())
		FSlateApplication::Get().OnPostTick().Remove(PostTickDelegateHandle);
}

void UFairyApplication::BeginDestroy()
{
	OnDestroy();

	// 从静态表中移除自身
	for (auto It = Instances.CreateIterator(); It; ++It)
	{
		if (It.Value().Get() == this)
		{
			It.RemoveCurrent();
			break;
		}
	}

	Super::BeginDestroy();
}

UGRoot* UFairyApplication::GetUIRoot() const
{
	if (UIRoot == nullptr)
	{
		UFairyApplication* This = const_cast<UFairyApplication*>(this);
		This->UIRoot = NewObject<UGRoot>(This);
		This->UIRoot->AddToViewport();
	}

	return UIRoot;
}

void UFairyApplication::CallAfterSlateTick(FSimpleDelegate Callback)
{
	PostTickMulticastDelegate.Add(Callback);
}

void UFairyApplication::OnSlatePostTick(float DeltaTime)
{
	if (PostTickMulticastDelegate.IsBound())
	{
		FSimpleMulticastDelegate Clone = PostTickMulticastDelegate;
		PostTickMulticastDelegate.Clear();
		Clone.Broadcast();
	}

	if (bNeedCheckPopups)
	{
		bNeedCheckPopups = false;

		if (UIRoot != nullptr)
			UIRoot->CheckPopups(nullptr);
	}
}

FVector2D UFairyApplication::GetTouchPosition(int32 InUserIndex, int32 InPointerIndex)
{
	FTouchInfo* TouchInfo = GetTouchInfo(InUserIndex, InPointerIndex);
	if (TouchInfo != nullptr)
		return TouchInfo->Event.GetScreenSpacePosition();
	else
		return FVector2D::ZeroVector;
}

int32 UFairyApplication::GetTouchCount() const
{
	int32 Count = 0;
	for (auto& it : Touches)
	{
		if (it.bDown)
			Count++;
	}

	return Count;
}

UGObject* UFairyApplication::GetObjectUnderPoint(const FVector2D& ScreenspacePosition)
{
	UGRoot* Root = GetUIRoot();
	if (!Root)
		return nullptr;

	const SDisplayObject* Hit = Root->GetDisplayObject()->HitTest(ScreenspacePosition);
	if (!Hit)
		return nullptr;

	// 沿父链向上查找最近的有效 GObject
	const SDisplayObject* Ptr = Hit;
	while (Ptr)
	{
		if (Ptr->GObject.IsValid())
			return Ptr->GObject.Get();
		Ptr = Ptr->GetParent().Get();
	}
	return nullptr;
}

void UFairyApplication::CancelClick(int32 InUserIndex, int32 InPointerIndex)
{
	FTouchInfo* TouchInfo = GetTouchInfo(InUserIndex, InPointerIndex);
	if (TouchInfo != nullptr)
		TouchInfo->bClickCancelled = true;
}

void UFairyApplication::PlaySound(const FString& URL, float VolumnScale)
{
	if (!bSoundEnabled)
		return;

	TSharedPtr<FPackageItem> SoundItem = UUIPackage::GetItemByURL(URL);
	if (SoundItem.IsValid())
	{
		SoundItem->Load();
		if (SoundItem->Sound.IsValid())
			FSlateApplication::Get().PlaySound(*SoundItem->Sound);
	}
}

void UFairyApplication::SetSoundEnabled(bool bEnabled)
{
	bSoundEnabled = bEnabled;
}

void UFairyApplication::SetSoundVolumeScale(float VolumnScale)
{
	SoundVolumeScale = VolumnScale;
}

bool UFairyApplication::DispatchEvent(
	const FName& EventType, const TSharedRef<SDisplayObject>& Initiator, const FNVariant& Data)
{
	UGObject* Obj = SDisplayObject::GetGObject(Initiator);
	if (Obj == nullptr)
		return false;

	UEventContext* Context = BorrowEventContext();
	Context->Type = EventType;
	Context->Initiator = Obj;
	Context->Sender = Obj;
	Context->Data = Data;

	Context->Sender->InvokeEventDelegate(Context);

	ReturnEventContext(Context);

	return Context->bDefaultPrevented;
}

void UFairyApplication::BubbleEvent(
	const FName& EventType, const TSharedRef<SDisplayObject>& Initiator, const FNVariant& Data)
{
	TArray<UGObject*> CallChain;
	SDisplayObject::GetPathToRoot(Initiator, CallChain);
	if (CallChain.Num() == 0)
		return;

	InternalBubbleEvent(EventType, CallChain, Data);
}

void UFairyApplication::InternalBubbleEvent(
	const FName& EventType, const TArray<UGObject*>& CallChain, const FNVariant& Data)
{
	UEventContext* Context = BorrowEventContext();
	Context->Type = EventType;
	Context->Initiator = CallChain[0];
	Context->Data = Data;

	for (auto& it : CallChain)
	{
		Context->Sender = it;
		it->InvokeEventDelegate(Context);

		if (Context->bIsMouseCaptor)
		{
			Context->bIsMouseCaptor = false;
			AddMouseCaptor(Context->GetUserIndex(), (int32)Context->GetPointerIndex(), it);
		}

		if (Context->IsPropagationStopped())
			break;
	}

	ReturnEventContext(Context);
}

void UFairyApplication::BroadcastEvent(
	const FName& EventType, const TSharedRef<SDisplayObject>& Initiator, const FNVariant& Data)
{
	TArray<UGObject*> CallChain;
	SDisplayObject::GetDescendants(Initiator, CallChain);
	if (CallChain.Num() == 0)
		return;

	UEventContext* Context = BorrowEventContext();
	Context->Type = EventType;
	Context->Data = Data;

	for (auto& it : CallChain)
	{
		Context->Sender = it;
		Context->Initiator = it;
		it->InvokeEventDelegate(Context);
	}

	ReturnEventContext(Context);
}

UEventContext* UFairyApplication::BorrowEventContext()
{
	UEventContext* Context;
	if (EventContextPool.Num() > 0)
	{
		Context = EventContextPool.Pop();
		Context->bDefaultPrevented = false;
		Context->bStopped = false;
		Context->bIsMouseCaptor = false;
	}
	else
		Context = NewObject<UEventContext>(this);
	Context->PointerEvent = &LastTouch->Event;
	Context->ClickCount = LastTouch->ClickCount;
	// Context->KeyEvent = &LastKeyEvent;

	return Context;
}

void UFairyApplication::ReturnEventContext(UEventContext* Context)
{
	Context->Data.Reset();
	EventContextPool.Add(Context);
}

void UFairyApplication::AddMouseCaptor(int32 InUserIndex, int32 InPointerIndex, UGObject* InTarget)
{
	FTouchInfo* TouchInfo = GetTouchInfo(InUserIndex, InPointerIndex);
	if (TouchInfo != nullptr && !TouchInfo->MouseCaptors.Contains(InTarget))
		TouchInfo->MouseCaptors.Add(InTarget);
}

void UFairyApplication::RemoveMouseCaptor(int32 InUserIndex, int32 InPointerIndex, UGObject* InTarget)
{
	FTouchInfo* TouchInfo = GetTouchInfo(InUserIndex, InPointerIndex);
	if (TouchInfo != nullptr)
		TouchInfo->MouseCaptors.Remove(InTarget);
}

bool UFairyApplication::HasMouseCaptor(int32 InUserIndex, int32 InPointerIndex)
{
	FTouchInfo* TouchInfo = GetTouchInfo(InUserIndex, InPointerIndex);
	return TouchInfo != nullptr && TouchInfo->MouseCaptors.Num() > 0;
}

UFairyApplication::FTouchInfo* UFairyApplication::GetTouchInfo(const FPointerEvent& MouseEvent)
{
	FTouchInfo* TouchInfo = nullptr;
	for (auto& it : Touches)
	{
		if (it.UserIndex == MouseEvent.GetUserIndex() && it.PointerIndex == (int32)MouseEvent.GetPointerIndex())
		{
			TouchInfo = &it;
			break;
		}
	}

	if (TouchInfo == nullptr)
	{
		TouchInfo = new FTouchInfo();
		Touches.Add(TouchInfo);
		TouchInfo->UserIndex = MouseEvent.GetUserIndex();
		TouchInfo->PointerIndex = (int32)MouseEvent.GetPointerIndex();
	}

	LastTouch = TouchInfo;
	return TouchInfo;
}

UFairyApplication::FTouchInfo* UFairyApplication::GetTouchInfo(int32 InUserIndex, int32 InPointerIndex)
{
	if (InUserIndex == -1 && InPointerIndex == -1)
		return LastTouch;

	for (auto& it : Touches)
	{
		if (it.UserIndex == InUserIndex && it.PointerIndex == InPointerIndex)
		{
			return &it;
		}
	}
	return nullptr;
}

void UFairyApplication::PreviewDownEvent(const FPointerEvent& MouseEvent)
{
	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	TouchInfo->Event = MouseEvent;
	TouchInfo->bDown = true;
	TouchInfo->DownPosition = MouseEvent.GetScreenSpacePosition();
	TouchInfo->bClickCancelled = false;
	TouchInfo->ClickCount = 1;
	TouchInfo->MouseCaptors.Reset();
	TouchInfo->bToClearCaptors = false;
	TouchInfo->DownPath.Reset();

	bNeedCheckPopups = true;
}

void UFairyApplication::PreviewUpEvent(const FPointerEvent& MouseEvent)
{
	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	TouchInfo->Event = MouseEvent;
	TouchInfo->bDown = false;
	TouchInfo->bToClearCaptors = true;

	if (TouchInfo->MouseCaptors.Num() > 0)
	{
		FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

		int32 cnt = TouchInfo->MouseCaptors.Num();
		for (int32 i = 0; i < cnt; i++)
		{
			auto& Captor = TouchInfo->MouseCaptors[i];
			if (Captor.IsValid() && Captor->OnStage())
			{
				DispatchEvent(FUIEvents::TouchEnd, Captor->GetDisplayObject());
			}
		}
	}
}

void UFairyApplication::PreviewMoveEvent(const FPointerEvent& MouseEvent)
{
	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	TouchInfo->Event = MouseEvent;

	if ((TouchInfo->DownPosition - MouseEvent.GetScreenSpacePosition()).GetAbsMax() > 50)
		TouchInfo->bClickCancelled = true;

	if (!TouchInfo->bToClearCaptors && TouchInfo->MouseCaptors.Num() > 0)
	{
		FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

		int32 cnt = TouchInfo->MouseCaptors.Num();
		for (int32 i = 0; i < cnt; i++)
		{
			auto& Captor = TouchInfo->MouseCaptors[i];
			if (Captor.IsValid() && Captor->OnStage())
			{
				DispatchEvent(FUIEvents::TouchMove, Captor->GetDisplayObject());
			}
		}
	}
}

bool UFairyApplication::OnWidgetMouseButtonDown(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);

	UGObject*				   InitialGObject = nullptr;
	TSharedPtr<SDisplayObject> Ptr = DisplayObj;
	// SDisplayObject 已脱离 SWidget，父链终点不再是 ViewportWidget
	// 遍历到父链顶端（nullptr）或 UGRoot 为止
	while (Ptr.IsValid())
	{
		TouchInfo->DownPath.Add(Ptr);

		if (InitialGObject == nullptr)
		{
			InitialGObject = Ptr->GObject.Get();
		}
		Ptr = Ptr->GetParent();
	}

	bNeedCheckPopups = false;
	if (UIRoot != nullptr)
		UIRoot->CheckPopups(&DisplayObj.Get());

	BubbleEvent(FUIEvents::TouchBegin, DisplayObj);

	return true;
}

bool UFairyApplication::OnWidgetMouseButtonUp(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	if (TouchInfo == nullptr)
		return true;

	TArray<UGObject*> CallChain;
	SDisplayObject::GetPathToRoot(DisplayObj, CallChain);
	if (CallChain.Num() > 0)
	{
		for (auto& it : TouchInfo->MouseCaptors)
		{
			if (it.IsValid())
				CallChain.RemoveSingle(it.Get());
		}

		if (CallChain.Num() > 0)
			InternalBubbleEvent(FUIEvents::TouchEnd, CallChain, FNVariant::Null);
	}
	TouchInfo->MouseCaptors.Reset();

	if (!TouchInfo->bClickCancelled)
	{
		TSharedPtr<SDisplayObject> Ptr = DisplayObj;
		// SDisplayObject 已脱离 SWidget，父链终点不再是 ViewportWidget
		while (Ptr.IsValid())
		{
			if (TouchInfo->DownPath.Contains(Ptr))
			{
				BubbleEvent(FUIEvents::Click, Ptr.ToSharedRef());
				break;
			}
			Ptr = Ptr->GetParent();
		}
	}

	TouchInfo->DownPath.Reset();

	return true;
}

bool UFairyApplication::OnWidgetMouseMove(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	return true;
}

bool UFairyApplication::OnWidgetMouseButtonDoubleClick(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	TouchInfo->ClickCount = 2;

	return OnWidgetMouseButtonDown(DisplayObj, MouseEvent);
}

void UFairyApplication::OnWidgetMouseEnter(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	DispatchEvent(FUIEvents::RollOver, DisplayObj);
}

void UFairyApplication::OnWidgetMouseLeave(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	DispatchEvent(FUIEvents::RollOut, DisplayObj);
}

bool UFairyApplication::OnWidgetMouseWheel(
	const TSharedRef<SDisplayObject>& DisplayObj, const FPointerEvent& MouseEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());

	FTouchInfo* TouchInfo = GetTouchInfo(MouseEvent);
	TouchInfo->Event = MouseEvent;

	BubbleEvent(FUIEvents::MouseWheel, DisplayObj);

	return true;
}

bool UFairyApplication::OnWidgetKeyDown(
	const TSharedRef<SDisplayObject>& DisplayObj, const FKeyEvent& InKeyEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());
	return DispatchEvent(FUIEvents::KeyDown, DisplayObj, FNVariant(InKeyEvent.GetKey().ToString()));
}

bool UFairyApplication::OnWidgetKeyChar(
	const TSharedRef<SDisplayObject>& DisplayObj, const FCharacterEvent& InCharacterEvent)
{
	return false;
}

bool UFairyApplication::OnWidgetKeyUp(
	const TSharedRef<SDisplayObject>& DisplayObj, const FKeyEvent& InKeyEvent)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());
	return DispatchEvent(FUIEvents::KeyUp, DisplayObj, FNVariant(InKeyEvent.GetKey().ToString()));
}

bool UFairyApplication::OnWidgetFocusReceived(
	const TSharedRef<SDisplayObject>& DisplayObj, EFocusCause InCause)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());
	return DispatchEvent(FUIEvents::GetFocus, DisplayObj, FNVariant((int32)InCause));
}

void UFairyApplication::OnWidgetFocusLost(
	const TSharedRef<SDisplayObject>& DisplayObj, EFocusCause InCause)
{
	FMyScopedSwitchWorldHack SwitchWorld(GetWorld());
	DispatchEvent(FUIEvents::LostFocus, DisplayObj, FNVariant((int32)InCause));
}

UDragDropManager* UFairyApplication::GetDragDropManager() const
{
	return DragDropManager;
}

bool UFairyApplication::IsSoundEnabled() const
{
	return bSoundEnabled;
}

float UFairyApplication::GetSoundVolumeScale() const
{
	return SoundVolumeScale;
}