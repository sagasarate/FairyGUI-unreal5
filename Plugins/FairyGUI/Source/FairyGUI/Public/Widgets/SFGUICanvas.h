#pragma once
#include "CoreMinimal.h"
#include "Slate.h"
#include "Widgets/Renderer/FairyRenderPipeline.h"

class UGRoot;
class SDisplayObject;

class FAIRYGUI_API SFGUICanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFGUICanvas) {}
	SLATE_END_ARGS()

	SFGUICanvas();
	void Construct(const FArguments& InArgs);

	// UGRoot 双向访问
	void	SetUIRoot(UGRoot* InRoot) { m_UIRoot = InRoot; }
	UGRoot* GetUIRoot() const { return m_UIRoot.Get(); }

	// SWidget overrides (渲染)
	virtual void	  Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32	  OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D::ZeroVector; }

	// SWidget overrides (鼠标消息处理)
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void   OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void   OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// SWidget overrides (键盘消息处理)
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	virtual bool						  SupportsKeyboardFocus() const override { return true; }
	virtual TOptional<EMouseCursor::Type> GetCursor() const override;
	virtual FNavigationReply			  OnNavigation(
					 const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent) override;

	// 焦点管理
	void				  SetFocus(const SDisplayObject* InWidget, EFocusCause InCause = EFocusCause::SetDirectly);
	void				  ClearFocus();
	const SDisplayObject* GetFocusedObject() const { return m_FocusedObject; }

	// MouseCapture 机制
	static void						  CaptureMouse(const TSharedPtr<SDisplayObject>& InCaptor);
	static void						  ReleaseMouseCapture();
	static TSharedPtr<SDisplayObject> GetMouseCaptor();
	static bool						  HasMouseCapture() { return s_MouseCaptor.IsValid(); }

	// 命中测试入口
	const SDisplayObject* HitTest(const FVector2D& GlobalPoint) const;

private:
	TWeakObjectPtr<UGRoot>										  m_UIRoot;
	mutable TSharedPtr<FFairyRenderPipeline, ESPMode::ThreadSafe> m_Pipeline;

	static TWeakPtr<SDisplayObject> s_MouseCaptor;

	// Hover 追踪
	TArray<const SDisplayObject*> m_LastHoveredPath;
	static void					  BuildAncestorPath(const SDisplayObject* Leaf, TArray<const SDisplayObject*>& OutPath);

	// 焦点追踪
	const SDisplayObject* m_FocusedObject = nullptr;

	// Tab 导航：在 FGUI 树中查找下一个/上一个 TabStop
	const SDisplayObject* FindNextTabStop(const SDisplayObject* Current, bool bForward) const;
};
