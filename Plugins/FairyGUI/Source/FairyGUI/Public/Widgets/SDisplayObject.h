#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Slate.h"
#include "Utils/TypeID.h"
#include "Widgets/Renderer/RenderGroup.h"

class UGObject;

enum class EDisplayObjChildLayer
{
	Front,
	Back,
};

class FAIRYGUI_API SDisplayObject : public TSharedFromThis<SDisplayObject>
{
protected:
	enum class EAttrChangeFlag : uint32
	{
		Create = 1,
		Position = (1 << 1),
		Size = (1 << 2),
		Scale = (1 << 3),
		Rotation = (1 << 4),
		Skew = (1 << 5),
		ChangeParent = (1 << 6),
		Visible = (1 << 7),
	};
	bool m_bVisible : 1 = true;
	bool m_bInteractable : 1 = true;
	bool m_bTouchable : 1 = true;
	bool m_bOpaque : 1 = true;
	bool m_bRebuildMesh : 1 = false;
	bool m_bEnbaleHitTest : 1 = false;

	FVector2D m_Position;
	FVector2D m_Size;
	FVector2D m_Scale;
	FVector2D m_Skew;
	float	  m_Rotation = 0;

	FColor			m_Color = FColor::White;
	float			m_Saturation = 1.0f;
	EFairyBlendMode m_BlendMode = EFairyBlendMode::Normal;

	float m_RenderOpacity = 1.0f;

	TOptional<EMouseCursor::Type> m_MouseCursor;
	bool						  m_bFocusable = false;
	bool						  m_bTabStop = true;

	// 自有渲染变换（替代 SWidget 的 RenderTransform）
	TOptional<FSlateRenderTransform> m_RenderTransform;
	FVector2D						 m_RenderTransformPivot = FVector2D(0.5f, 0.5f);

	// 是否裁剪到边界
	bool m_bClipToBounds = false;

	// 裁剪边界扩展：防止溢出内容被错误剔除（等效原 SWidget::SetCullingBoundsExtension）
	FMargin m_CullingBoundsExtension;

	TWeakPtr<SDisplayObject> m_pParent = nullptr;

	bool m_bForceUpdateGeometry = false;

	mutable uint32 m_AttrChangeFlag = (uint32)EAttrChangeFlag::Create;

	mutable FSlateRenderTransform m_CachedInverseDesktopTransform;

	// 自有几何缓存（替代 SWidget 的 PersistentState）
	mutable FGeometry m_CachedGeometry;		   // 画布空间（等效 SWidget::AllottedGeometry / GetPaintSpaceGeometry）
	mutable FGeometry m_CachedDesktopGeometry; // 桌面空间（等效 SWidget::GetDesktopSpaceGeometry）

public:
	// 窗口→桌面偏移，由 SFGUICanvas::OnPaint 每帧更新，供 CaculateGeometry 计算 DesktopGeometry
	static FVector2D s_WindowToDesktopOffset;
	static FName	 s_TypeName;
	virtual FName	 GetTypeName() const { return s_TypeName; }
	static FName	 GetTypeNameStatic();
	virtual bool	 IsTypeOf(FName TypeName) const { return s_TypeName == TypeName ? true : false; }
	template <class T>
	bool IsA() const { return IsTypeOf(T::GetTypeNameStatic()); }

	TWeakObjectPtr<class UGObject> GObject;

	explicit SDisplayObject(UGObject* InGObject = nullptr);
	virtual ~SDisplayObject() = default;

	void SetParent(TSharedPtr<SDisplayObject> pParent)
	{
		if (m_pParent != pParent)
		{
			m_pParent = pParent;
			m_AttrChangeFlag |= (uint32)EAttrChangeFlag::ChangeParent;
		}
	}
	TSharedPtr<SDisplayObject>		 GetParent() { return m_pParent.Pin(); }
	TSharedPtr<const SDisplayObject> GetParent() const { return TSharedPtr<const SDisplayObject>(m_pParent.Pin()); }

	const FVector2D GetPosition() const;
	void			SetPosition(const FVector2D& InPosition);

	float GetX();
	void  SetX(float InX);
	float GetY();
	void  SetY(float InY);
	void  SetXY(float InX, float InY);

	void SetSize(const FVector2D& InSize);
	void SetSize(float Width, float Height);

	void			 SetWidth(float Width);
	void			 SetHeight(float Height);
	const FVector2D& GetSize() const { return m_Size; }
	float			 GetWidth() const { return m_Size.X; }
	float			 GetHeight() const { return m_Size.Y; }

	bool			 SetScale(const FVector2D& InScale);
	bool			 SetScale(float ScaleX, float ScaleY);
	bool			 SetScaleX(float ScaleX);
	bool			 SetScaleY(float ScaleY);
	const FVector2D& GetScale() const { return m_Scale; }
	float			 GetScaleX() const { return m_Scale.X; }
	float			 GetScaleY() const { return m_Scale.Y; }

	bool			 SetSkew(const FVector2D& InSkew);
	const FVector2D& GetSkew() const { return m_Skew; }

	bool  SetRotation(float InRotation);
	float GetRotation() const { return m_Rotation; }

	void		  SetColor(const FColor& InColor);
	const FColor& GetColor() const { return m_Color; }

	void SetVisible(bool bInVisible);
	bool IsVisible() const { return m_bVisible; }

	void SetTouchable(bool bInTouchable);
	bool IsTouchable() const { return m_bTouchable; }

	void SetOpaque(bool bInOpaque);
	bool IsOpaque() const { return m_bOpaque; }

	bool IsHitTestVisible() const { return m_bInteractable && m_bTouchable; }
	bool AreChildrenHitTestVisible() const { return m_bInteractable && m_bTouchable; }

	void SetForceUpdateGeometry(bool bForce) { m_bForceUpdateGeometry = bForce; }
	bool IsForceUpdateGeometry() const { return m_bForceUpdateGeometry; }

	void		 SetInteractable(bool bInInteractable);
	virtual bool IsInteractable() const { return m_bInteractable; }

	void EnableHitTest(bool bEnable);
	bool IsHitTestEnabled() const { return m_bEnbaleHitTest; }

	void  SetSaturation(float InSaturation) { m_Saturation = InSaturation; }
	float GetSaturation() const { return m_Saturation; }

	void			SetBlendMode(EFairyBlendMode InBlendMode) { m_BlendMode = InBlendMode; }
	EFairyBlendMode GetBlendMode() const { return m_BlendMode; }

	// 渲染属性（替代 SWidget）
	void  SetRenderOpacity(float InOpacity) { m_RenderOpacity = InOpacity; }
	float GetRenderOpacity() const { return m_RenderOpacity; }

	// 几何缓存访问
	const FGeometry& GetPaintSpaceGeometry() const { return m_CachedGeometry; }
	const FGeometry& GetDesktopSpaceGeometry() const { return m_CachedDesktopGeometry; }

	void ForceUpdateGeometry() { m_AttrChangeFlag |= (uint32)EAttrChangeFlag::ChangeParent; };

	// 桌面空间变换矩阵，直接使用避免 FGeometry 内部重复计算逆矩阵
	const FSlateRenderTransform& GetDesktopTransform() const
	{
		return m_CachedDesktopGeometry.GetAccumulatedRenderTransform();
	}
	const FSlateRenderTransform& GetInverseDesktopTransform() const { return m_CachedInverseDesktopTransform; }

	// 渲染变换（自有实现）
	const TOptional<FSlateRenderTransform>& GetRenderTransform() const { return m_RenderTransform; }
	void									SetRenderTransform(const FSlateRenderTransform& InTransform) { m_RenderTransform = InTransform; }
	FVector2D								GetRenderTransformPivot() const { return m_RenderTransformPivot; }
	void									SetRenderTransformPivot(const FVector2D& InPivot) { m_RenderTransformPivot = InPivot; }

	// 裁剪
	bool IsClipToBounds() const { return m_bClipToBounds; }
	void SetClipToBounds(bool bEnable) { m_bClipToBounds = bEnable; }

	// 裁剪边界扩展
	const FMargin& GetCullingBoundsExtension() const { return m_CullingBoundsExtension; }
	void		   SetCullingBoundsExtension(const FMargin& InMargin) { m_CullingBoundsExtension = InMargin; }

	void								 SetMouseCursor(const TOptional<EMouseCursor::Type>& Cursor) { m_MouseCursor = Cursor; }
	const TOptional<EMouseCursor::Type>& GetMouseCursor() const { return m_MouseCursor; }

	void SetFocusable(bool bEnable) { m_bFocusable = bEnable; }
	bool IsFocusable() const { return m_bFocusable; }

	void SetTabStop(bool bEnable) { m_bTabStop = bEnable; }
	bool IsTabStop() const { return m_bTabStop; }

	void UpdateVisibilityFlags();

	// 渲染收集（不再接收 FPaintArgs）
	virtual void CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
		float InSaturation) const;

	// 鼠标事件（去掉 override，保留签名供将来 SFGUICanvas 调用）
	virtual bool OnMouseButtonDown(const FPointerEvent& MouseEvent);
	virtual bool OnMouseButtonUp(const FPointerEvent& MouseEvent);
	virtual bool OnMouseMove(const FPointerEvent& MouseEvent);
	virtual bool OnMouseButtonDoubleClick(const FPointerEvent& MouseEvent);
	virtual void OnMouseEnter(const FPointerEvent& MouseEvent);
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent);
	virtual bool OnMouseWheel(const FPointerEvent& MouseEvent);

	// 键盘和焦点事件
	virtual bool OnKeyDown(const FKeyEvent& InKeyEvent);
	virtual bool OnKeyChar(const FCharacterEvent& InCharacterEvent);
	virtual bool OnKeyUp(const FKeyEvent& InKeyEvent);
	virtual bool OnFocusReceived(EFocusCause InCause);
	virtual void OnFocusLost(EFocusCause InCause);

	virtual TOptional<EMouseCursor::Type> GetCursor() const;
	virtual bool						  SupportsKeyboardFocus() const;
	virtual bool						  OnNavigation(const FNavigationEvent& InNavigationEvent);

	static bool		 IsOnStage(const TSharedPtr<SDisplayObject>& InDisplayObj);
	static UGObject* GetGObject(const TSharedPtr<SDisplayObject>& InDisplayObj);
	static UGObject* GetGObjectIfOnStage(const TSharedPtr<SDisplayObject>& InDisplayObj);
	static void		 GetDescendants(const TSharedRef<SDisplayObject>& InDisplayObj, TArray<UGObject*>& OutArray);
	static void		 GetPathToRoot(const TSharedRef<SDisplayObject>& InDisplayObj, TArray<UGObject*>& OutArray);

	// 键盘导航：递归遍历容器树，查找第一个/最后一个 TabStop 控件
	static TSharedPtr<SDisplayObject> FindTabStopInTree(class SContainer* Container, bool bForward);
	// 键盘导航：在容器直接子控件中查找下一个/上一个 TabStop
	static TSharedPtr<SDisplayObject> FindNextTabStopChild(
		class SContainer* Container, const TSharedRef<SDisplayObject>& CurrentChild, bool bForward);

	virtual const SDisplayObject* HitTest(const FVector2D& GlobalPoint) const;

	void CaculateGeometry(const FGeometry& ParentAllottedGeometry) const;

protected:
	virtual void OnSizeChanged() {}

	void UpdateRenderTransform();
	bool NeedUpdateGeometryInherited() const;
};
