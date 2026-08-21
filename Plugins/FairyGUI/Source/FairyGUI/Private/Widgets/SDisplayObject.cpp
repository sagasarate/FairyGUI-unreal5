#include "Widgets/SDisplayObject.h"
#include "Widgets/SContainer.h"
#include "Widgets/SRichTextField.h"
#include "FairyApplication.h"
#include "Engine/GameViewportClient.h"
#include "UI/GObject.h"
#include "UI/GRoot.h"

FVector2D SDisplayObject::s_WindowToDesktopOffset = FVector2D::ZeroVector;

FName SDisplayObject::s_TypeName(TEXT("SDisplayObject"));
FName SDisplayObject::GetTypeNameStatic()
{
	return s_TypeName;
}

SDisplayObject::SDisplayObject(UGObject* InGObject) : m_Position(ForceInit), m_Size(ForceInit), m_Scale(1.0f, 1.0f)
{
	GObject = InGObject;
}

// 方案B: Position 独立存储于 m_Position，不再依赖 RenderTransform 的 translation
const FVector2D SDisplayObject::GetPosition() const
{
	return m_Position;
}

void SDisplayObject::SetPosition(const FVector2D& InPosition)
{
	if (!m_Position.Equals(InPosition))
	{
		m_Position = InPosition;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Position;
	}
}
float SDisplayObject::GetX()
{
	return m_Position.X;
}
void SDisplayObject::SetX(float InX)
{
	if (!FMath::IsNearlyEqual(m_Position.X, InX))
	{
		m_Position.X = InX;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Position;
	}
}
float SDisplayObject::GetY()
{
	return m_Position.Y;
}
void SDisplayObject::SetY(float InY)
{
	if (!FMath::IsNearlyEqual(m_Position.Y, InY))
	{
		m_Position.Y = InY;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Position;
	}
}
void SDisplayObject::SetXY(float InX, float InY)
{
	if (!FMath::IsNearlyEqual(m_Position.X, InX) || !FMath::IsNearlyEqual(m_Position.Y, InY))
	{
		m_Position.X = InX;
		m_Position.Y = InY;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Position;
	}
}

void SDisplayObject::SetSize(const FVector2D& InSize)
{
	if (m_Size != InSize)
	{
		m_Size = InSize;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Size;
		m_bRebuildMesh = true;
		OnSizeChanged();
	}
}

void SDisplayObject::SetSize(float Width, float Height)
{
	if (!FMath::IsNearlyEqual(m_Size.X, Width) || !FMath::IsNearlyEqual(m_Size.Y, Height))
	{
		m_Size.X = Width;
		m_Size.Y = Height;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Size;
		m_bRebuildMesh = true;
		OnSizeChanged();
	}
}
void SDisplayObject::SetWidth(float Width)
{
	if (!FMath::IsNearlyEqual(m_Size.X, Width))
	{
		m_Size.X = Width;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Size;
		m_bRebuildMesh = true;
		OnSizeChanged();
	}
}
void SDisplayObject::SetHeight(float Height)
{
	if (!FMath::IsNearlyEqual(m_Size.Y, Height))
	{
		m_Size.Y = Height;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Size;
		m_bRebuildMesh = true;
		OnSizeChanged();
	}
}
bool SDisplayObject::SetScale(const FVector2D& InScale)
{
	if (!m_Scale.Equals(InScale))
	{
		m_Scale = InScale;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Scale;
		UpdateRenderTransform();
		return true;
	}
	return false;
}
bool SDisplayObject::SetScale(float ScaleX, float ScaleY)
{
	if (!FMath::IsNearlyEqual(m_Scale.X, ScaleX) || !FMath::IsNearlyEqual(m_Scale.Y, ScaleY))
	{
		m_Scale.X = ScaleX;
		m_Scale.Y = ScaleY;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Scale;
		UpdateRenderTransform();
		return true;
	}
	return false;
}
bool SDisplayObject::SetScaleX(float ScaleX)
{
	if (!FMath::IsNearlyEqual(m_Scale.X, ScaleX))
	{
		m_Scale.X = ScaleX;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Scale;
		UpdateRenderTransform();
		return true;
	}
	return false;
}
bool SDisplayObject::SetScaleY(float ScaleY)
{
	if (!FMath::IsNearlyEqual(m_Scale.Y, ScaleY))
	{
		m_Scale.Y = ScaleY;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Scale;
		UpdateRenderTransform();
		return true;
	}
	return false;
}
bool SDisplayObject::SetSkew(const FVector2D& InSkew)
{
	if (!m_Skew.Equals(InSkew))
	{
		m_Skew = InSkew;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Skew;
		UpdateRenderTransform();
		return true;
	}
	return false;
}
bool SDisplayObject::SetRotation(float InRotation)
{
	if (!FMath::IsNearlyEqual(m_Rotation, InRotation))
	{
		m_Rotation = InRotation;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Rotation;
		UpdateRenderTransform();
		return true;
	}
	return false;
}
void SDisplayObject::SetColor(const FColor& InColor)
{
	if (m_Color != InColor)
	{
		m_Color = InColor;
		m_bRebuildMesh = true;
	}
}

void SDisplayObject::SetVisible(bool bInVisible)
{
	if (m_bVisible != bInVisible)
	{
		m_bVisible = bInVisible;
		m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Visible;
	}
}

void SDisplayObject::SetTouchable(bool bInTouchable)
{
	m_bTouchable = bInTouchable;
}

void SDisplayObject::SetOpaque(bool bInOpaque)
{
	m_bOpaque = bInOpaque;
}

void SDisplayObject::SetInteractable(bool bInInteractable)
{
	m_bInteractable = bInInteractable;
}

void SDisplayObject::EnableHitTest(bool bEnable)
{
	m_bEnbaleHitTest = bEnable;
}

void SDisplayObject::UpdateVisibilityFlags()
{
	// ponytail: 简化版，不再设置 Slate EVisibility；Bool 标志直接生效
	m_AttrChangeFlag |= (uint32)EAttrChangeFlag::Visible;
}

void SDisplayObject::UpdateRenderTransform()
{
	FScale2D   Scale2D = FScale2D(m_Scale);
	FQuat2D	   Quat2D = FQuat2D(FMath::DegreesToRadians(m_Rotation));
	FMatrix2x2 Matrix = Concatenate(Quat2D, Scale2D);

	// FairyGUI skew: 不是标准 shear(tan)，而是 sin/cos 变换，保持边长不变
	if (!FMath::IsNearlyZero(m_Skew.X) || !FMath::IsNearlyZero(m_Skew.Y))
	{
		float SX = m_Skew.X * UE_PI / 180.0f;
		float SY = m_Skew.Y * UE_PI / 180.0f;
		float SinX = FMath::Sin(SX);
		float CosX = FMath::Cos(SX);
		float SinY = FMath::Sin(SY);
		float CosY = FMath::Cos(SY);

		FMatrix2x2 S(CosY, SinY, -SinX, CosX);
		Matrix = Concatenate(S, Matrix);
	}

	// 方案B: RenderTransform 只存变换矩阵，translation 为 0；Position 由 m_Position 独立管理
	m_RenderTransform = FSlateRenderTransform(Matrix, FVector2D::ZeroVector);
}

bool SDisplayObject::NeedUpdateGeometryInherited() const
{
	if (m_AttrChangeFlag != 0)
		return true;
	auto ParentWidget = m_pParent.Pin();
	if (!ParentWidget.IsValid())
		return false;
	return ParentWidget->NeedUpdateGeometryInherited();
}

void SDisplayObject::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
	float InSaturation) const
{
	m_AttrChangeFlag = 0;
}

const SDisplayObject* SDisplayObject::HitTest(const FVector2D& GlobalPoint) const
{
	FBox2D ContentRect(FVector2D::ZeroVector, m_Size);
	auto   LocalPoint = m_CachedInverseDesktopTransform.TransformPoint(GlobalPoint);
	if (ContentRect.IsInside(LocalPoint))
		return this;
	return nullptr;
}

void SDisplayObject::CaculateGeometry(const FGeometry& ParentAllottedGeometry) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SDisplayObject_CaculateGeometry);

	if (NeedUpdateGeometryInherited())
	{
		FSlateRenderTransform RenderTransform;
		if (m_RenderTransform.IsSet())
		{
			RenderTransform = FSlateRenderTransform(m_RenderTransform->GetMatrix(), FVector2D::ZeroVector);
		}

		// 画布空间几何（等效 SWidget::AllottedGeometry / GetPaintSpaceGeometry）
		const FGeometry Geometry = ParentAllottedGeometry.MakeChild(
			m_Size, FSlateLayoutTransform(m_Position), RenderTransform, m_RenderTransformPivot);

		// 桌面空间几何（等效 SWidget::DesktopGeometry / GetDesktopSpaceGeometry）
		// 参考 SWidget::Paint: DesktopSpaceGeometry = AllottedGeometry;
		//                        DesktopSpaceGeometry.AppendTransform(WindowToDesktopTransform);
		FGeometry DesktopGeometry = Geometry;
		DesktopGeometry.AppendTransform(FSlateLayoutTransform(s_WindowToDesktopOffset));

		// 存入自有几何缓存
		m_CachedGeometry = Geometry;
		m_CachedDesktopGeometry = DesktopGeometry;

		// 缓存逆变换（桌面空间→本地空间），供 HitTest() 使用
		m_CachedInverseDesktopTransform = DesktopGeometry.GetAccumulatedRenderTransform().Inverse();
	}
}

bool SDisplayObject::OnMouseButtonDown(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		return Obj->GetApp()->OnWidgetMouseButtonDown(DisplayObj, MouseEvent);
	else
		return false;
}

bool SDisplayObject::OnMouseButtonUp(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		return Obj->GetApp()->OnWidgetMouseButtonUp(DisplayObj, MouseEvent);
	else
		return false;
}

bool SDisplayObject::OnMouseMove(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		return Obj->GetApp()->OnWidgetMouseMove(DisplayObj, MouseEvent);
	else
		return false;
}

bool SDisplayObject::OnMouseButtonDoubleClick(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		return Obj->GetApp()->OnWidgetMouseButtonDoubleClick(DisplayObj, MouseEvent);
	else
		return false;
}

void SDisplayObject::OnMouseEnter(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		Obj->GetApp()->OnWidgetMouseEnter(DisplayObj, MouseEvent);
}

void SDisplayObject::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		Obj->GetApp()->OnWidgetMouseLeave(DisplayObj, MouseEvent);
}

bool SDisplayObject::OnMouseWheel(const FPointerEvent& MouseEvent)
{
	auto	  DisplayObj = SharedThis(this);
	UGObject* Obj = GetGObject(DisplayObj);
	if (Obj != nullptr)
		return Obj->GetApp()->OnWidgetMouseWheel(DisplayObj, MouseEvent);
	else
		return false;
}

bool SDisplayObject::IsOnStage(const TSharedPtr<SDisplayObject>& InDisplayObj)
{
	TSharedPtr<SDisplayObject> Ptr = InDisplayObj;
	while (Ptr.IsValid())
	{
		if (Ptr->GObject.IsValid() && Ptr->GObject->IsA<UGRoot>())
			return true;
		Ptr = Ptr->GetParent();
	}

	return false;
}

bool SDisplayObject::OnKeyDown(const FKeyEvent& InKeyEvent)
{
	UGObject* Obj = GetGObject(SharedThis(this));
	return Obj ? Obj->GetApp()->OnWidgetKeyDown(SharedThis(this), InKeyEvent) : false;
}

bool SDisplayObject::OnKeyChar(const FCharacterEvent& InCharacterEvent)
{
	UGObject* Obj = GetGObject(SharedThis(this));
	return Obj ? Obj->GetApp()->OnWidgetKeyChar(SharedThis(this), InCharacterEvent) : false;
}

bool SDisplayObject::OnKeyUp(const FKeyEvent& InKeyEvent)
{
	UGObject* Obj = GetGObject(SharedThis(this));
	return Obj ? Obj->GetApp()->OnWidgetKeyUp(SharedThis(this), InKeyEvent) : false;
}

bool SDisplayObject::OnFocusReceived(EFocusCause InCause)
{
	UGObject* Obj = GetGObject(SharedThis(this));
	return Obj ? Obj->GetApp()->OnWidgetFocusReceived(SharedThis(this), InCause) : false;
}

void SDisplayObject::OnFocusLost(EFocusCause InCause)
{
	UGObject* Obj = GetGObject(SharedThis(this));
	if (Obj)
		Obj->GetApp()->OnWidgetFocusLost(SharedThis(this), InCause);
}

TOptional<EMouseCursor::Type> SDisplayObject::GetCursor() const
{
	return m_MouseCursor;
}

bool SDisplayObject::SupportsKeyboardFocus() const
{
	return m_bFocusable && m_bInteractable && m_bTouchable && m_bVisible;
}

bool SDisplayObject::OnNavigation(const FNavigationEvent& InNavigationEvent)
{
	// Tab 导航由 SFGUICanvas::OnKeyDown 统一处理
	return false;
}

TSharedPtr<SDisplayObject> SDisplayObject::FindTabStopInTree(SContainer* Container, bool bForward)
{
	if (Container == nullptr)
		return nullptr;

	const int32 Count = Container->NumChildren();

	for (int32 i = 0; i < Count; ++i)
	{
		const int32				   Idx = bForward ? i : (Count - 1 - i);
		TSharedPtr<SDisplayObject> DisplayChild = Container->GetChildAt(Idx);
		if (!DisplayChild.IsValid())
			continue;

		if (DisplayChild->IsTabStop() && DisplayChild->SupportsKeyboardFocus())
			return DisplayChild;

		if (DisplayChild->IsA<SContainer>())
		{
			auto Found = FindTabStopInTree(static_cast<SContainer*>(DisplayChild.Get()), bForward);
			if (Found.IsValid())
				return Found;
		}
	}

	return nullptr;
}

TSharedPtr<SDisplayObject> SDisplayObject::FindNextTabStopChild(
	SContainer* Container, const TSharedRef<SDisplayObject>& CurrentChild, bool bForward)
{
	if (Container == nullptr)
		return nullptr;

	const int32 Count = Container->NumChildren();
	if (Count == 0)
		return nullptr;

	int32 CurrentIndex = -1;
	for (int32 i = 0; i < Count; ++i)
	{
		TSharedPtr<SDisplayObject> Child = Container->GetChildAt(i);
		if (Child.IsValid() && Child == CurrentChild)
		{
			CurrentIndex = i;
			break;
		}
	}

	const int32 StartIdx = (CurrentIndex != -1) ? CurrentIndex : (bForward ? -1 : Count);
	for (int32 Step = 1; Step <= Count; ++Step)
	{
		const int32 Idx = bForward ? (StartIdx + Step) % Count : (StartIdx - Step + Count) % Count;

		TSharedPtr<SDisplayObject> DisplayChild = Container->GetChildAt(Idx);
		if (DisplayChild.IsValid() && DisplayChild->IsTabStop() && DisplayChild->SupportsKeyboardFocus())
			return DisplayChild;
	}

	return nullptr;
}

void SDisplayObject::GetDescendants(const TSharedRef<SDisplayObject>& InDisplayObj, TArray<UGObject*>& OutArray)
{
	if (InDisplayObj->GObject.IsValid())
		OutArray.Add(InDisplayObj->GObject.Get());

	if (InDisplayObj->IsA<SContainer>())
	{
		SContainer* Container = static_cast<SContainer*>(&InDisplayObj.Get());
		for (int32 i = 0; i < Container->NumChildren(); ++i)
		{
			TSharedPtr<SDisplayObject> Child = Container->GetChildAt(i);
			if (Child.IsValid())
				GetDescendants(Child.ToSharedRef(), OutArray);
		}
		return;
	}
	if (InDisplayObj->IsA<SRichTextField>())
	{
		SRichTextField* RichText = static_cast<SRichTextField*>(&InDisplayObj.Get());
		for (int32 i = 0; i < RichText->NumChildren(); ++i)
		{
			TSharedPtr<SDisplayObject> Child = RichText->GetChildAt(i);
			if (Child.IsValid())
				GetDescendants(Child.ToSharedRef(), OutArray);
		}
		return;
	}
}

UGObject* SDisplayObject::GetGObject(const TSharedPtr<SDisplayObject>& InDisplayObj)
{
	TSharedPtr<SDisplayObject> Ptr = InDisplayObj;
	while (Ptr.IsValid())
	{
		if (Ptr->GObject.IsValid())
			return Ptr->GObject.Get();
		Ptr = Ptr->GetParent();
	}
	return nullptr;
}

UGObject* SDisplayObject::GetGObjectIfOnStage(const TSharedPtr<SDisplayObject>& InDisplayObj)
{
	TSharedPtr<SDisplayObject> Ptr = InDisplayObj;
	UGObject*				   Result = nullptr;
	while (Ptr.IsValid())
	{
		if (Result == nullptr)
			Result = Ptr->GObject.Get();
		if (Ptr->GObject.IsValid() && Ptr->GObject->IsA<UGRoot>())
			return Result;
		Ptr = Ptr->GetParent();
	}

	return nullptr;
}

void SDisplayObject::GetPathToRoot(const TSharedRef<SDisplayObject>& InDisplayObj, TArray<UGObject*>& OutArray)
{
	TSharedPtr<SDisplayObject> Ptr = InDisplayObj;
	while (Ptr.IsValid())
	{
		// ponytail: 不再使用 Advanced_IsWindow()（SWidget 方法），改为检查 GObject 是否 UGRoot
		if (Ptr->GObject.IsValid())
		{
			if (Ptr->GObject->IsA<UGRoot>())
				break;
			OutArray.Add(Ptr->GObject.Get());
		}
		Ptr = Ptr->GetParent();
	}
}
