#include "UI/GRoot.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Slate.h"
#include "FairyApplication.h"
#include "UI/GWindow.h"
#include "UI/GGraph.h"
#include "UI/UIPackage.h"
#include "Widgets/SContainer.h"
#include "Utils/ObjectPool.h"
#include "Widgets/SFGUICanvas.h"

int32 UGRoot::ContentScaleLevel = 0;

UGRoot* UGRoot::Get(UObject* WorldContextObject)
{
	return UFairyApplication::Get(WorldContextObject)->GetUIRoot();
}

UGRoot::UGRoot() {}

UGRoot::~UGRoot() {}

void UGRoot::Dispose()
{
	if (bDisposed)
		return;

	// 释放对象池中的 UI 对象
	if (IsValid(ObjectPool))
	{
		ObjectPool->DisposeAllObjects();
		ObjectPool = nullptr;
	}

	// 释放独立持有的 UI 对象（可能不在 Children 树中，Dispose 幂等）
	if (IsValid(ModalLayer))
		ModalLayer->Dispose();
	if (IsValid(ModalWaitPane))
		ModalWaitPane->Dispose();
	if (IsValid(TooltipWin))
		TooltipWin->Dispose();
	if (IsValid(DefaultTooltipWin))
		DefaultTooltipWin->Dispose();
	for (auto& Popup : PopupStack)
	{
		if (Popup.IsValid())
			Popup->Dispose();
	}
	PopupStack.Empty();
	for (auto& Popup : JustClosedPopups)
	{
		if (Popup.IsValid())
			Popup->Dispose();
	}
	JustClosedPopups.Empty();

	// 清除 tooltip 定时器
	if (ShowTooltipsTimerHandle.IsValid())
		ShowTooltipsTimerHandle.Invalidate();

	// 从视口移除全屏画布，防止视口继续持有引用
	if (FullScreenCanvas.IsValid())
	{
		UFairyApplication* App = GetApp();
		if (App && App->GetViewportClient())
			App->GetViewportClient()->RemoveViewportWidgetContent(FullScreenCanvas.ToSharedRef());
		FullScreenCanvas.Reset();
	}
	UGComponent::Dispose(); // 处理 RootContainer/Container 然后递归子节点
}

void UGRoot::AddToViewport()
{
	UGameViewportClient* ViewportClient = GetApp()->GetViewportClient();
	verifyf(ViewportClient != nullptr, TEXT("Null Viewport?"));

	FullScreenCanvas = SNew(SFGUICanvas);
	FullScreenCanvas->SetUIRoot(this);
	ViewportClient->AddViewportWidgetContent(FullScreenCanvas.ToSharedRef(), 100);

	// 尽早从父控件获取视口大小同步给 GRoot（Tick 会在此后持续同步）
	TSharedPtr<SWidget> ParentWidget = FullScreenCanvas->GetParentWidget();
	if (ParentWidget.IsValid())
	{
		SetSize(ParentWidget->GetPaintSpaceGeometry().GetLocalSize().RoundToVector());
	}
}

void UGRoot::ReturnFocus()
{
	FSlateApplication::Get().SetUserFocus(0, FullScreenCanvas);
}

void UGRoot::ClearFocus()
{
	// 归还 Slate 键盘焦点，防止窗口关闭等路径未走输入框 OnFocusLost 时按键失去响应
	FSlateApplication::Get().SetAllUserFocusToGameViewport();

	if (FullScreenCanvas.IsValid())
	{
		FullScreenCanvas->ClearFocus();
	}
}

UGObject* UGRoot::GetFocusedObject() const
{
	if (!FullScreenCanvas.IsValid())
	{
		return nullptr;
	}

	const SDisplayObject* FocusedDisp = FullScreenCanvas->GetFocusedObject();
	if (!FocusedDisp)
	{
		return nullptr;
	}

	return SDisplayObject::GetGObject(const_cast<SDisplayObject*>(FocusedDisp)->AsShared());
}

void UGRoot::ShowWindow(UGWindow* Window)
{
	AddChild(Window);
	AdjustModalLayer();
}

void UGRoot::HideWindow(UGWindow* Window)
{
	Window->Hide();
}

void UGRoot::HideWindowImmediately(UGWindow* Window)
{
	if (Window->GetParent() == this)
		RemoveChild(Window);

	AdjustModalLayer();
}

void UGRoot::BringToFront(UGWindow* Window)
{
	int32 cnt = NumChildren();
	int32 i;
	if (ModalLayer->GetParent() != nullptr && !Window->IsModal())
		i = GetChildIndex(ModalLayer) - 1;
	else
		i = cnt - 1;

	for (; i >= 0; i--)
	{
		UGObject* g = GetChildAt(i);
		if (g == Window)
			return;
		if (g->IsA<UGWindow>())
			break;
	}

	if (i >= 0)
		SetChildIndex(Window, i);
}

void UGRoot::CloseAllExceptModals()
{
	TArray<UGObject*> map;
	map.Append(Children);

	for (const auto& child : map)
	{
		if (child->IsA<UGWindow>() && !((UGWindow*)child)->IsModal())
			HideWindowImmediately((UGWindow*)child);
	}
}

void UGRoot::CloseAllWindows()
{
	TArray<UGObject*> map;
	map.Append(Children);

	for (const auto& child : map)
	{
		if (child->IsA<UGWindow>())
			HideWindowImmediately((UGWindow*)child);
	}
}

UGWindow* UGRoot::GetTopWindow() const
{
	int32 cnt = NumChildren();
	for (int32 i = cnt - 1; i >= 0; i--)
	{
		UGObject* child = GetChildAt(i);
		if (child->IsA<UGWindow>())
		{
			return (UGWindow*)child;
		}
	}

	return nullptr;
}

UGGraph* UGRoot::GetModalLayer()
{
	if (ModalLayer == nullptr)
		CreateModalLayer();

	return ModalLayer;
}

void UGRoot::CreateModalLayer()
{
	ModalLayer = NewObject<UGGraph>(this);
	ModalLayer->SetSize(Size);
	ModalLayer->DrawRect(0, FColor::White, FUIConfig::Config.ModalLayerColor);
	ModalLayer->AddRelation(this, ERelationType::Size);
}

void UGRoot::AdjustModalLayer()
{
	if (ModalLayer == nullptr)
		CreateModalLayer();

	int32 cnt = NumChildren();

	if (ModalWaitPane != nullptr && ModalWaitPane->GetParent() != nullptr)
		SetChildIndex(ModalWaitPane, cnt - 1);

	for (int32 i = cnt - 1; i >= 0; i--)
	{
		UGObject* child = GetChildAt(i);
		if (child->IsA<UGWindow>() && ((UGWindow*)child)->IsModal())
		{
			if (ModalLayer->GetParent() == nullptr)
				AddChildAt(ModalLayer, i);
			else
				SetChildIndexBefore(ModalLayer, i);
			return;
		}
	}

	if (ModalLayer->GetParent() != nullptr)
		RemoveChild(ModalLayer);
}

bool UGRoot::HasModalWindow() const
{
	return ModalLayer != nullptr && ModalLayer->GetParent() != nullptr;
}

void UGRoot::ShowModalWait()
{
	GetModalWaitingPane();
	if (ModalWaitPane)
		AddChild(ModalWaitPane);
}

void UGRoot::CloseModalWait()
{
	if (ModalWaitPane != nullptr && ModalWaitPane->GetParent() != nullptr)
		RemoveChild(ModalWaitPane);
}

UGObject* UGRoot::GetModalWaitingPane()
{
	if (!FUIConfig::Config.GlobalModalWaiting.IsEmpty())
	{
		if (ModalWaitPane == nullptr)
		{
			ModalWaitPane = UUIPackage::CreateObjectFromURL(FUIConfig::Config.GlobalModalWaiting, this);
			ModalWaitPane->SetSortingOrder(INT_MAX);
		}

		ModalWaitPane->SetSize(GetSize());
		ModalWaitPane->AddRelation(this, ERelationType::Size);

		return ModalWaitPane;
	}
	else
		return nullptr;
}

bool UGRoot::IsModalWaiting() const
{
	return (ModalWaitPane != nullptr) && ModalWaitPane->OnStage();
}

void UGRoot::ShowPopup(UGObject* Popup, UGObject* AtObject, EPopupDirection Direction)
{
	if (PopupStack.Num() > 0)
		HidePopup(Popup);

	PopupStack.Add(Popup);

	if (AtObject != nullptr)
	{
		UGObject* p = AtObject;
		while (p != nullptr)
		{
			if (p->GetParent() == this)
			{
				if (Popup->GetSortingOrder() < p->GetSortingOrder())
				{
					Popup->SetSortingOrder(p->GetSortingOrder());
				}
				break;
			}
			p = p->GetParent();
		}
	}

	AddChild(Popup);
	AdjustModalLayer();

	if (Popup->IsA<UGWindow>() && AtObject == nullptr && Direction == EPopupDirection::Auto)
		return;

	FVector2D pos = GetPoupPosition(Popup, AtObject, Direction);
	Popup->SetPosition(pos);
}

void UGRoot::TogglePopup(UGObject* Popup, UGObject* AtObject, EPopupDirection Direction)
{
	int32 Index;
	if (JustClosedPopups.Find(Popup, Index))
		return;

	ShowPopup(Popup, AtObject, Direction);
}

void UGRoot::HidePopup(UGObject* Popup)
{
	if (Popup != nullptr)
	{
		int32 k;
		if (PopupStack.Find(Popup, k))
		{
			for (int32 i = PopupStack.Num() - 1; i >= k; i--)
			{
				ClosePopup(PopupStack.Last().Get());
				PopupStack.Pop();
			}
		}
	}
	else
	{
		for (const auto& it : PopupStack)
			ClosePopup(it.Get());
		PopupStack.Reset();
	}
}

void UGRoot::ClosePopup(UGObject* Popup)
{
	if (Popup != nullptr && Popup->GetParent() != nullptr)
	{
		if (Popup->IsA<UGWindow>())
			((UGWindow*)Popup)->Hide();
		else
			RemoveChild(Popup);
	}
}

void UGRoot::CheckPopups(SDisplayObject* ClickTarget)
{
	JustClosedPopups.Reset();
	if (PopupStack.Num() > 0)
	{
		bool			handled = false;
		SDisplayObject* Ptr = ClickTarget;
		SDisplayObject* Top = DisplayObject.Get();
		while (Ptr != Top && Ptr != nullptr)
		{
			UGObject* Obj = static_cast<SDisplayObject*>(Ptr)->GObject.Get();

			int32 k;
			if (PopupStack.Find(Obj, k))
			{
				for (int32 i = PopupStack.Num() - 1; i > k; i--)
				{
					ClosePopup(PopupStack.Pop().Get());
				}
				handled = true;
				break;
			}
			Ptr = Ptr->GetParent().Get();
		}

		if (!handled)
		{
			for (int32 i = PopupStack.Num() - 1; i >= 0; i--)
			{
				UGObject* popup = PopupStack[i].Get();
				if (popup != nullptr)
				{
					JustClosedPopups.Add(popup);
					ClosePopup(popup);
				}
			}
			PopupStack.Reset();
		}
	}
}

bool UGRoot::HasAnyPopup() const
{
	return PopupStack.Num() > 0;
}

FVector2D UGRoot::GetPoupPosition(UGObject* Popup, UGObject* AtObject, EPopupDirection Direction)
{
	FVector2D pos;
	FVector2D size(0, 0);
	if (AtObject != nullptr)
	{
		pos = AtObject->LocalToGlobal(FVector2D::ZeroVector);
		pos = this->GlobalToLocal(pos);
		size = AtObject->LocalToGlobal(AtObject->GetSize());
		size = this->GlobalToLocal(size);
		size -= pos;
	}
	else
	{
		pos = GlobalToLocal(GetApp()->GetTouchPosition());
	}
	FVector2D RetPosition;
	RetPosition.X = pos.X;
	if (RetPosition.X + Popup->GetWidth() > GetWidth())
		RetPosition.X += size.X - Popup->GetWidth();
	RetPosition.Y = pos.Y + size.Y;
	if ((Direction == EPopupDirection::Auto && RetPosition.Y + Popup->GetHeight() > GetHeight())
		|| Direction == EPopupDirection::Up)
	{
		RetPosition.Y = pos.Y - Popup->GetHeight() - 1;
		if (RetPosition.Y < 0)
		{
			RetPosition.Y = 0;
			RetPosition.X += size.X / 2;
		}
	}

	return RetPosition.RoundToVector();
}

void UGRoot::ShowTooltips(const FText& Text)
{
	if (DefaultTooltipWin == nullptr)
	{
		const FString& resourceURL = FUIConfig::Config.TooltipsWin;
		if (resourceURL.IsEmpty())
		{
			UE_LOG(LogFairyGUI, Warning, TEXT("UIConfig.tooltipsWin not defined"));
			return;
		}

		DefaultTooltipWin = UUIPackage::CreateObjectFromURL(resourceURL, this);
		DefaultTooltipWin->SetTouchable(false);
	}

	DefaultTooltipWin->SetText(Text);
	ShowTooltipsWin(DefaultTooltipWin);
}

void UGRoot::ShowTooltipsWin(UGObject* InTooltipWin)
{
	HideTooltips();

	TooltipWin = InTooltipWin;
	GetWorld()->GetTimerManager().SetTimer(
		ShowTooltipsTimerHandle, FTimerDelegate::CreateUObject(this, &UGRoot::DoShowTooltipsWin), 0.1f, false);
}

void UGRoot::DoShowTooltipsWin()
{
	if (TooltipWin == nullptr)
		return;

	FVector2D pt = GetApp()->GetTouchPosition();
	FVector2D Pos = pt + FVector2D(10, 20);

	Pos = GlobalToLocal(Pos);

	if (Pos.X + TooltipWin->GetWidth() > GetWidth())
		Pos.X -= TooltipWin->GetWidth();
	if (Pos.Y + TooltipWin->GetHeight() > GetHeight())
	{
		Pos.Y -= TooltipWin->GetHeight() - 1;
		if (Pos.Y < 0)
			Pos.Y = 0;
	}

	TooltipWin->SetPosition(Pos.RoundToVector());
	AddChild(TooltipWin);
}

void UGRoot::HideTooltips()
{
	if (TooltipWin != nullptr)
	{
		if (TooltipWin->GetParent() != nullptr)
			RemoveChild(TooltipWin);
		TooltipWin = nullptr;
	}
}

UObjectPool* UGRoot::GetObjectPool()
{
	if (!IsValid(ObjectPool))
		ObjectPool = NewObject<UObjectPool>(this);
	return ObjectPool;
}
