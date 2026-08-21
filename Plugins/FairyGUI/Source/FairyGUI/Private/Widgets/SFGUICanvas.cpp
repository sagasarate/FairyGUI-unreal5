#include "Widgets/SFGUICanvas.h"
#include "FairyCommons.h"
#include "UI/GRoot.h"
#include "UI/GObject.h"
#include "Widgets/SDisplayObject.h"
#include "Widgets/SContainer.h"

TWeakPtr<SDisplayObject> SFGUICanvas::s_MouseCaptor = nullptr;

SFGUICanvas::SFGUICanvas()
{
	SetCanTick(true);
}

void SFGUICanvas::Construct(const SFGUICanvas::FArguments& InArgs)
{
	// 由 UGRoot::AddToViewport 在创建后调用 SetUIRoot 设置 FGUI 树根
}

void SFGUICanvas::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	UGRoot* Root = m_UIRoot.Get();
	if (Root)
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize().RoundToVector();
		if (LocalSize != Root->GetSize())
		{
			Root->SetSize(LocalSize);
			UE_LOG(LogFairyGUI, Log, TEXT("UIRoot resize to %f,%f (via Tick)"), LocalSize.X, LocalSize.Y);
		}
	}
}

int32 SFGUICanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SFGUICanvas_OnPaint);

	UGRoot* Root = m_UIRoot.Get();
	if (!Root)
		return LayerId;

	// 每帧更新窗口→桌面偏移，供 CaculateGeometry 计算 DesktopGeometry
	auto Offset = Args.GetWindowToDesktopTransform();
	if (!Offset.Equals(SDisplayObject::s_WindowToDesktopOffset))
	{
		SDisplayObject::s_WindowToDesktopOffset = Offset;
		Root->GetDisplayObject()->ForceUpdateGeometry();
	}

	auto pRootGroup = FRenderGroup::Borrow();
	pRootGroup->bHasClipping = false;
	FSlateRect FloatRect = AllottedGeometry.GetRenderBoundingRect();
	pRootGroup->ClippingRect = FIntRect(FMath::FloorToInt(FloatRect.Left), FMath::FloorToInt(FloatRect.Top),
		FMath::CeilToInt(FloatRect.Right), FMath::CeilToInt(FloatRect.Bottom));

	const TSharedRef<SDisplayObject> RootDisplay = Root->GetDisplayObject();
	if (RootDisplay->IsVisible())
	{
		RootDisplay->CaculateGeometry(AllottedGeometry);
		RootDisplay->CollectRenderUnits(pRootGroup, RootDisplay->GetPaintSpaceGeometry(),
			GetRenderOpacity() * RootDisplay->GetRenderOpacity(), RootDisplay->GetSaturation());
	}

	if (m_Pipeline.IsValid())
		m_Pipeline->ReturnPendingGroups();

	if (!pRootGroup->Children.IsEmpty())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(SFGUICanvas_Render);
		if (!m_Pipeline.IsValid())
			m_Pipeline = MakeShared<FFairyRenderPipeline, ESPMode::ThreadSafe>();
		m_Pipeline->SetRootGroup(pRootGroup);
		FSlateDrawElement::MakeCustom(OutDrawElements, LayerId + 1, m_Pipeline);
	}
	else
	{
		FRenderGroup::Return(pRootGroup);
	}

	const_cast<SFGUICanvas*>(this)->Invalidate(EInvalidateWidgetReason::Paint);

	return LayerId;
}

// ---- MouseCapture ----

void SFGUICanvas::CaptureMouse(const TSharedPtr<SDisplayObject>& InCaptor)
{
	s_MouseCaptor = InCaptor;
}

void SFGUICanvas::ReleaseMouseCapture()
{
	s_MouseCaptor.Reset();
}

TSharedPtr<SDisplayObject> SFGUICanvas::GetMouseCaptor()
{
	return s_MouseCaptor.Pin();
}

TOptional<EMouseCursor::Type> SFGUICanvas::GetCursor() const
{
	if (auto Captor = s_MouseCaptor.Pin())
	{
		if (Captor->GetMouseCursor().IsSet())
			return Captor->GetMouseCursor();
	}
	if (m_LastHoveredPath.Num() > 0)
	{
		const SDisplayObject* Hovered = m_LastHoveredPath.Last();
		if (Hovered->GetMouseCursor().IsSet())
			return Hovered->GetMouseCursor();
	}
	return TOptional<EMouseCursor::Type>();
}

// ---- HitTest ----

const SDisplayObject* SFGUICanvas::HitTest(const FVector2D& GlobalPoint) const
{
	UGRoot* Root = m_UIRoot.Get();
	if (!Root)
		return nullptr;

	return Root->GetDisplayObject()->HitTest(GlobalPoint);
}

// ---- 鼠标消息分发 ----

static bool DispatchMouseEvent(
	const SDisplayObject* Target, const FPointerEvent& MouseEvent,
	bool (SDisplayObject::*Handler)(const FPointerEvent&))
{
	if (!Target)
		return false;
	return (const_cast<SDisplayObject*>(Target)->*Handler)(MouseEvent);
}

static FReply ToSlateReply(bool bHandled)
{
	return bHandled ? FReply::Handled() : FReply::Unhandled();
}

void SFGUICanvas::BuildAncestorPath(const SDisplayObject* Leaf, TArray<const SDisplayObject*>& OutPath)
{
	OutPath.Reset();
	for (const SDisplayObject* Ptr = Leaf; Ptr; Ptr = Ptr->GetParent().Get())
	{
		OutPath.Add(Ptr);
	}
	// 反转：根在前，叶在后（匹配 UE Slate 的 WidgetPath 顺序）
	Algo::Reverse(OutPath);
}

static void UpdateHoverPath(
	TArray<const SDisplayObject*>& LastPath, const TArray<const SDisplayObject*>& CurPath,
	const FPointerEvent& MouseEvent)
{
	// 反向遍历旧路径（叶到根）：不在新路径中的 → OnMouseLeave
	for (int32 i = LastPath.Num() - 1; i >= 0; --i)
	{
		const SDisplayObject* OldWidget = LastPath[i];
		if (!CurPath.Contains(OldWidget))
		{
			const_cast<SDisplayObject*>(OldWidget)->OnMouseLeave(MouseEvent);
		}
	}

	// 正向遍历新路径（根到叶）：不在旧路径中的 → OnMouseEnter
	for (int32 i = 0; i < CurPath.Num(); ++i)
	{
		const SDisplayObject* NewWidget = CurPath[i];
		if (!LastPath.Contains(NewWidget))
		{
			const_cast<SDisplayObject*>(NewWidget)->OnMouseEnter(MouseEvent);
		}
	}

	// 更新缓存路径
	LastPath = CurPath;
}

FReply SFGUICanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const SDisplayObject* Target = HitTest(MouseEvent.GetScreenSpacePosition());
	if (Target)
	{
		bool bHandled = DispatchMouseEvent(Target, MouseEvent, &SDisplayObject::OnMouseButtonDown);
		if (Target->IsFocusable())
			SetFocus(Target, EFocusCause::Mouse);
		else
			ClearFocus();
		return ToSlateReply(bHandled);
	}
	// 点击空白处清除焦点
	ClearFocus();
	return FReply::Unhandled();
}

FReply SFGUICanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (auto Captor = s_MouseCaptor.Pin())
	{
		bool bHandled = Captor->OnMouseButtonUp(MouseEvent);
		if (!Captor->IsA<SContainer>())
			s_MouseCaptor.Reset();
		return ToSlateReply(bHandled);
	}

	const SDisplayObject* Target = HitTest(MouseEvent.GetScreenSpacePosition());
	if (Target)
		return ToSlateReply(DispatchMouseEvent(Target, MouseEvent, &SDisplayObject::OnMouseButtonUp));
	return FReply::Unhandled();
}

FReply SFGUICanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (auto Captor = s_MouseCaptor.Pin())
		return ToSlateReply(Captor->OnMouseMove(MouseEvent));

	const SDisplayObject* Target = HitTest(MouseEvent.GetScreenSpacePosition());

	TArray<const SDisplayObject*> CurPath;
	if (Target)
		BuildAncestorPath(Target, CurPath);
	UpdateHoverPath(m_LastHoveredPath, CurPath, MouseEvent);

	if (Target)
		return ToSlateReply(DispatchMouseEvent(Target, MouseEvent, &SDisplayObject::OnMouseMove));
	return FReply::Unhandled();
}

FReply SFGUICanvas::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const SDisplayObject* Target = HitTest(MouseEvent.GetScreenSpacePosition());
	if (Target)
		return ToSlateReply(DispatchMouseEvent(Target, MouseEvent, &SDisplayObject::OnMouseButtonDoubleClick));
	return FReply::Unhandled();
}

void SFGUICanvas::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const SDisplayObject*		  Target = HitTest(MouseEvent.GetScreenSpacePosition());
	TArray<const SDisplayObject*> CurPath;
	if (Target)
		BuildAncestorPath(Target, CurPath);
	UpdateHoverPath(m_LastHoveredPath, CurPath, MouseEvent);
}

void SFGUICanvas::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	// 鼠标离开 Canvas：清空整条路径
	TArray<const SDisplayObject*> EmptyPath;
	UpdateHoverPath(m_LastHoveredPath, EmptyPath, MouseEvent);
}

FReply SFGUICanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const SDisplayObject* Target = HitTest(MouseEvent.GetScreenSpacePosition());
	if (Target)
		return ToSlateReply(DispatchMouseEvent(Target, MouseEvent, &SDisplayObject::OnMouseWheel));
	return FReply::Unhandled();
}

// ---- 键盘消息分发 ----

FReply SFGUICanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// ESC：先发给焦点控件，未处理则清除焦点
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (m_FocusedObject)
		{
			bool bHandled = const_cast<SDisplayObject*>(m_FocusedObject)->OnKeyDown(InKeyEvent);
			if (bHandled)
				return FReply::Handled();
		}
		ClearFocus();
		return FReply::Handled();
	}

	// Tab：先发给焦点控件，未处理则做焦点导航
	if (InKeyEvent.GetKey() == EKeys::Tab)
	{
		if (m_FocusedObject)
		{
			bool bHandled = const_cast<SDisplayObject*>(m_FocusedObject)->OnKeyDown(InKeyEvent);
			if (bHandled)
				return FReply::Handled();
		}

		const bool			  bForward = !InKeyEvent.IsShiftDown();
		const SDisplayObject* Next = FindNextTabStop(m_FocusedObject, bForward);
		if (Next)
			SetFocus(Next, EFocusCause::Navigation);
		else
			ClearFocus();
		return FReply::Handled();
	}

	if (m_FocusedObject)
	{
		const_cast<SDisplayObject*>(m_FocusedObject)->OnKeyDown(InKeyEvent);
		// 画布挂在 SViewport 的 ChildSlot 内（AddViewportWidgetContent → ViewportOverlayWidget），
		// 焦点路径必然包含 SViewport。焦点对象未处理的按键（如游戏快捷键）若继续冒泡，
		// 会到达 SViewport::OnKeyDown 被引擎当作 InputAction 响应，因此只要 FairyGUI 持有焦点就一律消费
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SFGUICanvas::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	if (m_FocusedObject)
		return ToSlateReply(const_cast<SDisplayObject*>(m_FocusedObject)->OnKeyChar(InCharacterEvent));
	return FReply::Unhandled();
}

FReply SFGUICanvas::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (m_FocusedObject)
	{
		const_cast<SDisplayObject*>(m_FocusedObject)->OnKeyUp(InKeyEvent);
		// 同 OnKeyDown：阻止按键松开事件冒泡到 SViewport，避免触发 InputAction 的 Completed 阶段
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---- 焦点管理（模拟 UE Slate::SetUserFocus） ----

void SFGUICanvas::SetFocus(const SDisplayObject* InWidget, EFocusCause InCause)
{
	if (InWidget == m_FocusedObject)
		return;

	const SDisplayObject* OldFocus = m_FocusedObject;
	m_FocusedObject = (InWidget && InWidget->SupportsKeyboardFocus()) ? InWidget : nullptr;

	if (OldFocus)
		const_cast<SDisplayObject*>(OldFocus)->OnFocusLost(InCause);

	if (m_FocusedObject)
		const_cast<SDisplayObject*>(m_FocusedObject)->OnFocusReceived(InCause);
}

void SFGUICanvas::ClearFocus()
{
	SetFocus(nullptr, EFocusCause::Cleared);
}

// ---- Tab 导航 ----

const SDisplayObject* SFGUICanvas::FindNextTabStop(const SDisplayObject* Current, bool bForward) const
{
	UGRoot* Root = m_UIRoot.Get();
	if (!Root)
		return nullptr;

	struct FTabStopCollector
	{
		// 收集所有 TabStop，遇到 TabStopScope 容器时跳过其子树
		static void CollectSkipScope(const SDisplayObject* Node, TArray<const SDisplayObject*>& Out)
		{
			if (!Node)
				return;
			if (Node->IsTabStop() && Node->SupportsKeyboardFocus())
				Out.Add(Node);
			if (auto* Container = Node->IsA<SContainer>() ? static_cast<const SContainer*>(Node) : nullptr)
			{
				if (Container->IsTabStopScope())
					return;
				for (int32 i = 0; i < Container->NumChildren(); ++i)
				{
					TSharedPtr<SDisplayObject> Child = Container->GetChildAt(i);
					if (Child.IsValid())
						CollectSkipScope(Child.Get(), Out);
				}
			}
		}

		static void CollectAll(const SDisplayObject* Node, TArray<const SDisplayObject*>& Out)
		{
			if (!Node)
				return;
			if (Node->IsTabStop() && Node->SupportsKeyboardFocus())
				Out.Add(Node);
			if (auto* Container = Node->IsA<SContainer>() ? static_cast<const SContainer*>(Node) : nullptr)
			{
				for (int32 i = 0; i < Container->NumChildren(); ++i)
				{
					TSharedPtr<SDisplayObject> Child = Container->GetChildAt(i);
					if (Child.IsValid())
						CollectAll(Child.Get(), Out);
				}
			}
		}
	};

	// 查找最近的 TabStopScope 祖先容器
	const SDisplayObject* ScopeRoot = nullptr;
	for (const SDisplayObject* Ptr = Current; Ptr; Ptr = Ptr->GetParent().Get())
	{
		if (auto* Container = Ptr->IsA<SContainer>() ? static_cast<const SContainer*>(Ptr) : nullptr)
		{
			if (Container->IsTabStopScope())
			{
				ScopeRoot = Container;
				break;
			}
		}
	}

	TArray<const SDisplayObject*> AllStops;
	if (ScopeRoot)
	{
		FTabStopCollector::CollectAll(ScopeRoot, AllStops);
	}
	else if (Current)
	{
		const TSharedRef<SDisplayObject> RootDisp = Root->GetDisplayObject();
		FTabStopCollector::CollectSkipScope(&RootDisp.Get(), AllStops);
	}
	else
	{
		const TSharedRef<SDisplayObject> RootDisp = Root->GetDisplayObject();
		FTabStopCollector::CollectAll(&RootDisp.Get(), AllStops);
	}

	if (AllStops.Num() == 0)
		return nullptr;

	if (!Current)
		return bForward ? AllStops[0] : AllStops.Last();

	int32 CurIdx = AllStops.IndexOfByKey(Current);
	if (CurIdx == INDEX_NONE)
		return bForward ? AllStops[0] : AllStops.Last();

	int32 NextIdx = bForward ? CurIdx + 1 : CurIdx - 1;
	if (NextIdx < 0)
		NextIdx = AllStops.Num() - 1;
	else if (NextIdx >= AllStops.Num())
		NextIdx = 0;

	return AllStops[NextIdx];
}

FNavigationReply SFGUICanvas::OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent)
{
	return FNavigationReply::Stop();
}
