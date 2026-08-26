#include "UI/GObject.h"
#include "UI/GList.h"
#include "UI/GGroup.h"
#include "UI/GController.h"
#include "UI/UIPackage.h"
#include "UI/GRoot.h"
#include "UI/Gears/GearBase.h"
#include "UI/Gears/GearDisplay.h"
#include "UI/Gears/GearDisplay2.h"
#include "Widgets/SDisplayObject.h"
#include "Widgets/SFGUICanvas.h"
#include "Utils/ByteBuffer.h"
#include "FairyApplication.h"
#include "../Utils/Localization.h"
#include "FairyCommons.h"

TWeakObjectPtr<UGObject> UGObject::DraggingObject;
FVector2D				 UGObject::GlobalDragStart;
FBox2D					 UGObject::GlobalRect;
bool					 UGObject::bUpdateInDragging = false;

UGObject::UGObject()
	: SourceSize(ForceInit)
	, InitSize(ForceInit)
	, MinSize(ForceInit)
	, MaxSize(ForceInit)
	, Position(ForceInit)
	, Size(ForceInit)
	, RawSize(ForceInit)
	, Pivot(ForceInit)
	// , Scale{ 1, 1 }
	// , Skew(ForceInit)
	, Alpha(1.0f)
	, bVisible(true)
	, BlendMode(EFairyBlendMode::Normal)
	, bInternalVisible(true)
{
	static int32 _gInstanceCounter = 1;
	ID.AppendInt(_gInstanceCounter++);

	Relations = MakeShareable(new FRelations(this));
}

UGObject::~UGObject()
{
	// 安全网：若 Dispose() 未被调用（如对象不在 UI 树中、被 GC 直接回收），
	// 主动释放 DisplayObject 避免隐式成员析构触发 Slate widget 销毁链
	if (!bDisposed)
	{
		DisplayObject.Reset();
	}
}

void UGObject::Dispose()
{
	if (bDisposed)
		return;
	bDisposed = true;

	// 从显示树解绑
	RemoveFromParent();

	// 清除 C++ 事件委托，防止回调访问已释放资源
	EventDelegates.Empty();
	OnPositionChangedEvent.Clear();
	OnSizeChangedEvent.Clear();

	// 清除 Blueprint 绑定的动态委托
	OnClick.Clear();
	OnTouchBegin.Clear();
	OnTouchMove.Clear();
	OnTouchEnd.Clear();
	OnRollOver.Clear();
	OnRollOut.Clear();
	OnDragStart.Clear();
	OnDragMove.Clear();
	OnDragEnd.Clear();
	OnGearStop.Clear();
	OnAddedToStage.Clear();
	OnRemovedFromStage.Clear();
	OnKeyDown.Clear();
	OnKeyUp.Clear();

	// 释放 Relations（关联布局系统）
	if (Relations.IsValid())
		Relations->Dispose();

	// 释放 Gears（联动齿轮）
	for (auto& Gear : Gears)
	{
		if (Gear.IsValid())
			Gear->Dispose();
	}

	// 释放 DisplayObject（Slate 后端）—— 关键步骤
	// TSharedPtr refcount 归零时触发 Slate widget 析构链，
	// 此时静态 CIDStorage 池仍存活，归还操作安全
	DisplayObject.Reset();

	// 释放其他引用
	PackageItem.Reset();
	TreeNode = nullptr;
	CachedApp = nullptr;
	Group.Reset();
}

void UGObject::SetX(float InX)
{
	SetPosition(FVector2D(InX, Position.Y));
}

void UGObject::SetY(float InY)
{
	SetPosition(FVector2D(Position.X, InY));
}

void UGObject::SetPosition(const FVector2D& InPosition)
{
	if (Position != InPosition)
	{
		FVector2D delta = InPosition - Position;
		Position = InPosition;

		HandlePositionChanged();

		UGGroup* g = Cast<UGGroup>(this);
		if (g != nullptr)
			g->MoveChildren(delta);

		UpdateGear(1);

		if (Parent.IsValid() && !Parent->IsA<UGList>())
		{
			Parent->SetBoundsChangedFlag();
			if (Group.IsValid())
				Group->SetBoundsChangedFlag(true);

			OnPositionChangedEvent.Broadcast();
		}

		if (DraggingObject.Get() == this && !bUpdateInDragging)
			GlobalRect = LocalToGlobalRect(FBox2D(FVector2D(), Size));
	}
}

void UGObject::SetPositionInDragging(const FVector2D& InPosition)
{
	bUpdateInDragging = true;
	SetPosition(InPosition);
	bUpdateInDragging = false;
}

float UGObject::GetXMin() const
{
	return bPivotAsAnchor ? (Position.X - Size.X * Pivot.X) : Position.X;
}

void UGObject::SetXMin(float InXMin)
{
	if (bPivotAsAnchor)
		SetPosition(FVector2D(InXMin + Size.X * Pivot.X, Position.Y));
	else
		SetPosition(FVector2D(InXMin, Position.Y));
}

float UGObject::GetYMin() const
{
	return bPivotAsAnchor ? (Position.Y - Size.Y * Pivot.Y) : Position.Y;
}

void UGObject::SetYMin(float InYMin)
{
	if (bPivotAsAnchor)
		SetPosition(FVector2D(Position.X, InYMin + Size.Y * Pivot.Y));
	else
		SetPosition(FVector2D(Position.X, InYMin));
}

void UGObject::SetSize(const FVector2D& InSize, bool bIgnorePivot)
{
	if (RawSize != InSize)
	{
		RawSize = InSize;
		FVector2D ClampSize = InSize;
		if (ClampSize.X < MinSize.X)
			ClampSize.X = MinSize.X;
		else if (MaxSize.X > 0 && ClampSize.X > MaxSize.X)
			ClampSize.X = MaxSize.X;
		if (ClampSize.Y < MinSize.Y)
			ClampSize.Y = MinSize.Y;
		else if (MaxSize.Y > 0 && ClampSize.Y > MaxSize.Y)
			ClampSize.Y = MaxSize.Y;
		FVector2D Delta = ClampSize - Size;
		Size = ClampSize;

		HandleSizeChanged();

		if (Pivot.X != 0 || Pivot.Y != 0)
		{
			if (!bPivotAsAnchor)
			{
				if (!bIgnorePivot)
					SetPosition(Position - Pivot * Delta);
				else
					HandlePositionChanged();
			}
			else
				HandlePositionChanged();
		}

		UGGroup* g = Cast<UGGroup>(this);
		if (g != nullptr)
			g->ResizeChildren(Delta);

		UpdateGear(2);

		if (Parent.IsValid())
		{
			Relations->OnOwnerSizeChanged(Delta, bPivotAsAnchor || !bIgnorePivot);
			Parent->SetBoundsChangedFlag();
			if (Group.IsValid())
				Group->SetBoundsChangedFlag();
		}

		OnSizeChangedEvent.Broadcast();
	}
}

void UGObject::SetSizeDirectly(const FVector2D& InSize)
{
	RawSize = InSize;
	Size = InSize;
	if (Size.X < 0)
		Size.X = 0;
	if (Size.Y < 0)
		Size.Y = 0;
}

void UGObject::Center(bool bRestraint)
{
	UGComponent* r = nullptr;
	if (Parent.IsValid())
		r = Parent.Get();
	else
		r = GetUIRoot();

	SetPosition(((r->Size - Size) / 2).RoundToVector());
	if (bRestraint)
	{
		AddRelation(r, ERelationType::Center_Center);
		AddRelation(r, ERelationType::Middle_Middle);
	}
}

void UGObject::MakeFullScreen(bool bRestraint)
{
	SetSize(GetUIRoot()->GetSize());
	if (bRestraint)
		AddRelation(GetUIRoot(), ERelationType::Size);
}

void UGObject::SetPivot(const FVector2D& InPivot, bool bAsAnchor)
{
	if (Pivot != InPivot || bPivotAsAnchor != bAsAnchor)
	{
		Pivot = InPivot;
		bPivotAsAnchor = bAsAnchor;
		DisplayObject->SetRenderTransformPivot(FVector2D(Pivot.X, Pivot.Y));
		HandlePositionChanged();
	}
}

void UGObject::SetScale(const FVector2D& InScale)
{
	if (DisplayObject->SetScale(InScale))
		UpdateGear(2);
}

void UGObject::SetSkew(const FVector2D& InSkew)
{
	if (DisplayObject->SetSkew(InSkew))
		UpdateGear(2);
}

void UGObject::SetRotation(float InRotation)
{
	if (DisplayObject->SetRotation(InRotation))
		UpdateGear(3);
}

// void UGObject::UpdateTransform()
// {
// 	FScale2D   Scale2D = FScale2D(Scale);
// 	FQuat2D	   Quat2D = FQuat2D(FMath::DegreesToRadians(Rotation));
// 	FMatrix2x2 Matrix = Concatenate(Quat2D, Scale2D);

// 	// FairyGUI skew: 不是标准 shear(tan)，而是 sin/cos 变换，保持边长不变
// 	if (!FMath::IsNearlyZero(Skew.X) || !FMath::IsNearlyZero(Skew.Y))
// 	{
// 		float SX = Skew.X * UE_PI / 180.0f; // 转弧度
// 		float SY = Skew.Y * UE_PI / 180.0f;
// 		float SinX = FMath::Sin(SX);
// 		float CosX = FMath::Cos(SX);
// 		float SinY = FMath::Sin(SY);
// 		float CosY = FMath::Cos(SY);

// 		// S = [ cosY, sinY; -sinX, cosX ]，左乘于已有 Matrix:  M' = S * M
// 		FMatrix2x2 S(CosY, SinY, -SinX, CosX);
// 		Matrix = Concatenate(S, Matrix);
// 	}

// 	// 方案B: RenderTransform 只存变换矩阵，translation 为 0；Position 由 m_Position 独立管理
// 	DisplayObject->SetRenderTransform(FSlateRenderTransform(Matrix, FVector2D::ZeroVector));
// 	DisplayObject->SetForceUpdateGeometry(true);
// }

void UGObject::SetAlpha(float InAlpha)
{
	if (Alpha != InAlpha)
	{
		Alpha = InAlpha;
		HandleAlphaChanged();
		UpdateGear(3);
	}
}

void UGObject::SetGrayed(bool InBGrayed)
{
	if (bGrayed != InBGrayed)
	{
		bGrayed = InBGrayed;
		HandleGrayedChanged();
		UpdateGear(3);
	}
}

void UGObject::SetBlendMode(EFairyBlendMode InBlendMode)
{
	if (BlendMode != InBlendMode)
	{
		BlendMode = InBlendMode;
		HandleBlendModeChanged();
		UpdateGear(3);
	}
}

void UGObject::SetVisible(bool bInVisible)
{
	if (bVisible != bInVisible)
	{
		bVisible = bInVisible;
		HandleVisibleChanged();
		if (Parent.IsValid())
			Parent->SetBoundsChangedFlag();
		if (Group.IsValid() && Group->IsExcludeInvisibles())
			Group->SetBoundsChangedFlag();
	}
}

bool UGObject::InternalVisible() const
{
	return bInternalVisible && (!Group.IsValid() || Group->InternalVisible());
}

bool UGObject::InternalVisible2() const
{
	return bVisible && (!Group.IsValid() || Group->InternalVisible2());
}

bool UGObject::InternalVisible3() const
{
	return bVisible && bInternalVisible;
}

bool UGObject::IsTouchable() const
{
	return DisplayObject->IsTouchable();
}

void UGObject::SetTouchable(bool bInTouchable)
{
	DisplayObject->SetTouchable(bInTouchable);
}

void UGObject::SetSortingOrder(int32 InSortingOrder)
{
	if (InSortingOrder < 0)
		InSortingOrder = 0;
	if (SortingOrder != InSortingOrder)
	{
		int32 old = SortingOrder;
		SortingOrder = InSortingOrder;
		if (Parent.IsValid())
			Parent->ChildSortingOrderChanged(this, old, SortingOrder);
	}
}

void UGObject::SetGroup(UGGroup* InGroup)
{
	if (Group.Get() != InGroup)
	{
		if (Group.IsValid())
			Group->SetBoundsChangedFlag();
		Group = InGroup;
		if (Group.IsValid())
			Group->SetBoundsChangedFlag();
		HandleVisibleChanged();
		if (Parent.IsValid())
			Parent->ChildStateChanged(this);
	}
}

FText UGObject::GetText() const
{
	return G_EMPTY_TEXT;
}

void UGObject::SetText(const FText& InText) {}

const FString& UGObject::GetIcon() const
{
	return G_EMPTY_STRING;
}

void UGObject::SetIcon(const FString& InIcon) {}

void UGObject::SetTooltips(const FText& InTooltips)
{
	Tooltips = InTooltips;
	if (!Tooltips.IsEmpty())
	{
		On(FUIEvents::RollOver).AddUObject(this, &UGObject::OnRollOverHandler);
		On(FUIEvents::RollOut).AddUObject(this, &UGObject::OnRollOutHandler);
	}
}

void UGObject::OnRollOverHandler(UEventContext* Context)
{
	GetUIRoot()->ShowTooltips(Tooltips);
}

void UGObject::OnRollOutHandler(UEventContext* Context)
{
	GetUIRoot()->HideTooltips();
}

void UGObject::SetDraggable(bool bInDraggable)
{
	if (bDraggable != bInDraggable)
	{
		bDraggable = bInDraggable;
		InitDrag();
	}
}

FBox2D UGObject::GetDragBounds() const
{
	if (DragBounds.IsSet())
		return DragBounds.GetValue();
	else
		return FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
}

void UGObject::SetDragBounds(const FBox2D& InBounds)
{
	if (InBounds.Min == InBounds.Max && InBounds.Min == FVector2D::ZeroVector)
		DragBounds.Reset();
	else
		DragBounds = InBounds;
}

void UGObject::StartDrag(int32 UserIndex, int32 PointerIndex)
{
	DragBegin(UserIndex, PointerIndex);
}

void UGObject::StopDrag()
{
	DragEnd();
}

FString UGObject::GetResourceURL() const
{
	if (PackageItem.IsValid())
		return TEXT("ui://") + PackageItem->Owner->GetID() + PackageItem->ID;
	else
		return G_EMPTY_STRING;
}

FString UGObject::GetResourceName() const
{
	if (PackageItem.IsValid())
		return PackageItem->Name;
	else
		return G_EMPTY_STRING;
}

FString UGObject::GetPackageName() const
{
	if (PackageItem.IsValid())
		return PackageItem->Owner->GetName();
	else if (Parent.IsValid())
		return Parent->GetPackageName();
	else
		return G_EMPTY_STRING;
}

FVector2D UGObject::LocalToGlobal(const FVector2D& InPoint)
{
	FVector2D Point = InPoint;
	if (bPivotAsAnchor)
		Point += Size * Pivot;

	return DisplayObject->GetDesktopTransform().TransformPoint(Point);
}

FBox2D UGObject::LocalToGlobalRect(const FBox2D& InRect)
{
	FVector2D v0 = LocalToGlobal(InRect.Min);
	FVector2D v1 = LocalToGlobal(InRect.Max);
	return FBox2D(v0, v1);
}

FVector2D UGObject::LocalToRoot(const FVector2D& InPoint)
{
	return GetUIRoot()->GlobalToLocal(LocalToGlobal(InPoint));
}

FBox2D UGObject::LocalToRootRect(const FBox2D& InRect)
{
	return GetUIRoot()->GlobalToLocalRect(LocalToGlobalRect(InRect));
}

FVector2D UGObject::GlobalToLocal(const FVector2D& InPoint)
{
	FVector2D Point = DisplayObject->GetInverseDesktopTransform().TransformPoint(InPoint);
	if (bPivotAsAnchor)
		Point -= Size * Pivot;
	return Point;
}

FBox2D UGObject::GlobalToLocalRect(const FBox2D& InRect)
{
	FVector2D v0 = GlobalToLocal(InRect.Min);
	FVector2D v1 = GlobalToLocal(InRect.Max);
	return FBox2D(v0, v1);
}

FVector2D UGObject::RootToLocal(const FVector2D& InPoint)
{
	return GlobalToLocal(GetUIRoot()->LocalToGlobal(InPoint));
}

FBox2D UGObject::RootToLocalRect(const FBox2D& InRect)
{
	return GlobalToLocalRect(GetUIRoot()->LocalToGlobalRect(InRect));
}

void UGObject::AddRelation(UGObject* Obj, ERelationType RelationType, bool bUsePercent)
{
	verifyf(Obj != this, TEXT("Cannot add relation to self"));

	Relations->Add(Obj, RelationType, bUsePercent);
}

void UGObject::RemoveRelation(UGObject* Obj, ERelationType RelationType)
{
	Relations->Remove(Obj, RelationType);
}

const TSharedPtr<FGearBase>& UGObject::GetGear(int32 Index)
{
	TSharedPtr<FGearBase>& Gear = Gears[Index];
	if (!Gear.IsValid())
		Gear = FGearBase::Create(this, static_cast<FGearBase::EType>(Index));

	return Gear;
}

void UGObject::UpdateGear(int32 Index)
{
	if (bUnderConstruct || bGearLocked)
		return;

	const TSharedPtr<FGearBase>& Gear = Gears[Index];
	if (Gear.IsValid() && Gear->GetController() != nullptr)
		Gear->UpdateState();
}

bool UGObject::CheckGearController(int32 Index, UGController* Controller)
{
	return Gears[Index].IsValid() && Gears[Index]->GetController() == Controller;
}

void UGObject::UpdateGearFromRelations(int32 Index, const FVector2D& Delta)
{
	if (Gears[Index].IsValid())
		Gears[Index]->UpdateFromRelations(Delta);
}

uint32 UGObject::AddDisplayLock()
{
	const TSharedPtr<FGearDisplay>& GearDisplay = StaticCastSharedPtr<FGearDisplay>(Gears[0]);
	if (GearDisplay.IsValid() && GearDisplay->GetController() != nullptr)
	{
		uint32 ret = GearDisplay->AddLock();
		CheckGearDisplay();

		return ret;
	}
	else
		return 0;
}

void UGObject::ReleaseDisplayLock(uint32 Token)
{
	const TSharedPtr<FGearDisplay>& GearDisplay = StaticCastSharedPtr<FGearDisplay>(Gears[0]);
	if (GearDisplay.IsValid() && GearDisplay->GetController() != nullptr)
	{
		GearDisplay->ReleaseLock(Token);
		CheckGearDisplay();
	}
}

void UGObject::CheckGearDisplay()
{
	if (bHandlingController)
		return;

	bool bConnected = !Gears[0].IsValid() || StaticCastSharedPtr<FGearDisplay>(Gears[0])->IsConnected();
	if (Gears[8].IsValid() && Gears[8]->GetType() == FGearBase::EType::Display2)
		bConnected = StaticCastSharedPtr<FGearDisplay2>(Gears[8])->Evaluate(bConnected);

	if (bConnected != bInternalVisible)
	{
		bInternalVisible = bConnected;
		if (Parent.IsValid())
			Parent->ChildStateChanged(this);
		if (Group.IsValid() && Group->IsExcludeInvisibles())
			Group->SetBoundsChangedFlag();
	}
}

void UGObject::SetParent(UGObject* InParent)
{
	verifyf(InParent == nullptr || InParent->IsA<UGComponent>(), TEXT("Parent must be GComponent"));
	verifyf(InParent != this, TEXT("Parent must not be self"));

	if (InParent != nullptr)
		Cast<UGComponent>(InParent)->AddChild(this);
	else if (Parent.IsValid())
		Parent->RemoveChild(this);
}

void UGObject::SetParentToRoot()
{
	SetParent(GetUIRoot());
}

void UGObject::RemoveFromParent()
{
	if (Parent.IsValid())
		Parent->RemoveChild(this);
}

bool UGObject::OnStage() const
{
	return SDisplayObject::IsOnStage(DisplayObject);
}

UGRoot* UGObject::GetUIRoot() const
{
	return GetApp()->GetUIRoot();
}

UFairyApplication* UGObject::GetApp() const
{
	if (CachedApp == nullptr)
		const_cast<UGObject*>(this)->CachedApp = UFairyApplication::Get(const_cast<UGObject*>(this));

	return CachedApp;
}

UWorld* UGObject::GetWorld() const
{
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		return GetOuter()->GetWorld();
	else
		return nullptr;
}

UGObject* UGObject::CastTo(TSubclassOf<UGObject> ClassType) const
{
	return const_cast<UGObject*>(this);
}

FNVariant UGObject::GetProp(EObjectPropID PropID) const
{
	switch (PropID)
	{
		case EObjectPropID::Text:
			return FNVariant(GetText());
		case EObjectPropID::Icon:
			return FNVariant(GetIcon());
		default:
			return FNVariant(0);
	}
}

void UGObject::SetProp(EObjectPropID PropID, const FNVariant& InValue)
{
	switch (PropID)
	{
		case EObjectPropID::Text:
			return SetText(InValue.AsText());
		case EObjectPropID::Icon:
			return SetIcon(InValue.AsString());
		default:
			break;
	}
}

bool UGObject::DispatchEvent(const FName& EventType, const FNVariant& Data)
{
	return GetApp()->DispatchEvent(EventType, DisplayObject.ToSharedRef(), Data);
}

bool UGObject::HasEventListener(const FName& EventType) const
{
	const FUnifiedEventDelegate& Delegate = const_cast<UGObject*>(this)->GetEventDelegate(EventType);
	return Delegate.Func.IsBound() || (Delegate.DynFunc != nullptr && Delegate.DynFunc->IsBound());
}

void UGObject::InvokeEventDelegate(UEventContext* Context)
{
	FUnifiedEventDelegate& Delegate = GetEventDelegate(Context->GetType());
	Delegate.Func.Broadcast(Context);
	if (Delegate.DynFunc != nullptr)
		Delegate.DynFunc->Broadcast(Context);
}

UGObject::FUnifiedEventDelegate& UGObject::GetEventDelegate(const FName& EventType)
{
	FUnifiedEventDelegate* Delegate = EventDelegates.Find(EventType);
	if (Delegate == nullptr)
	{
		Delegate = &EventDelegates.Add(EventType);

#if (ENGINE_MAJOR_VERSION <= 4) && (ENGINE_MINOR_VERSION < 25)
		UProperty
#else
		FProperty
#endif
			* Property = GetClass()->FindPropertyByName(FName(*FString(TEXT("On")).Append(EventType.ToString())));

		if (Property != nullptr)
			Delegate->DynFunc = Property->ContainerPtrToValuePtr<FGUIEventDynMDelegate>(this);
		else
			Delegate->DynFunc = nullptr;
	}

	return *Delegate;
}

FGUIEventMDelegate& UGObject::On(const FName& EventType)
{
	return GetEventDelegate(EventType).Func;
}

void UGObject::ConstructFromResource() {}

float UGObject::GetX() const
{
	return Position.X;
}

float UGObject::GetY() const
{
	return Position.Y;
}

void UGObject::SetXY(float InX, float InY)
{
	SetPosition(FVector2D(InX, InY));
}

const FVector2D& UGObject::GetPosition() const
{
	return Position;
}

float UGObject::GetWidth() const
{
	return Size.X;
}

void UGObject::SetWidth(float InWidth)
{
	SetSize(FVector2D(InWidth, RawSize.Y));
}

float UGObject::GetHeight() const
{
	return Size.Y;
}

void UGObject::SetHeight(float InHeight)
{
	SetSize(FVector2D(RawSize.X, InHeight));
}

const FVector2D& UGObject::GetSize() const
{
	return Size;
}

const FVector2D& UGObject::GetPivot() const
{
	return Pivot;
}

bool UGObject::IsPivotAsAnchor() const
{
	return bPivotAsAnchor;
}

float UGObject::GetScaleX() const
{
	return DisplayObject->GetScaleX();
}

void UGObject::SetScaleX(float InScaleX)
{
	if (DisplayObject->SetScaleX(InScaleX))
		UpdateGear(2);
}

float UGObject::GetScaleY() const
{
	return DisplayObject->GetScaleY();
}

void UGObject::SetScaleY(float InScaleY)
{
	if (DisplayObject->SetScaleY(InScaleY))
		UpdateGear(2);
}

const FVector2D& UGObject::GetScale() const
{
	return DisplayObject->GetScale();
}

const FVector2D& UGObject::GetSkew() const
{
	return DisplayObject->GetSkew();
}

float UGObject::GetRotation() const
{
	return DisplayObject->GetRotation();
}

const FColor& UGObject::GetColor() const
{
	static FColor s_Default = FColor::White;
	if (DisplayObject.IsValid())
		return DisplayObject->GetColor();

	return s_Default;
}
void UGObject::SetColor(const FColor& InColor)
{
	if (DisplayObject.IsValid())
		DisplayObject->SetColor(InColor);
}

float UGObject::GetAlpha() const
{
	return Alpha;
}

EFairyBlendMode UGObject::GetBlendMode() const
{
	return BlendMode;
}

bool UGObject::IsGrayed() const
{
	return bGrayed;
}

bool UGObject::IsVisible() const
{
	return bVisible;
}

void UGObject::SetEnabled(bool bEnable)
{
	SetGrayed(!bEnable);
	SetTouchable(bEnable);
}

void UGObject::SetFocusable(bool bEnable)
{
	DisplayObject->SetFocusable(bEnable);
}

bool UGObject::IsFocusable() const
{
	return DisplayObject->IsFocusable();
}

void UGObject::SetTabStop(bool bEnable)
{
	DisplayObject->SetTabStop(bEnable);
}

bool UGObject::IsTabStop() const
{
	return DisplayObject->IsTabStop();
}

bool UGObject::Focused(bool bDescendant)
{
	if (!DisplayObject.IsValid())
		return false;

	TSharedPtr<SFGUICanvas> Canvas = GetUIRoot()->GetCanvas();
	if (!Canvas.IsValid())
		return false;

	TSharedPtr<const SDisplayObject> FocusedObj = Canvas->GetFocusedObject();
	if (!FocusedObj.IsValid())
		return false;

	if (FocusedObj.Get() == DisplayObject.Get())
		return true;

	if (bDescendant)
	{
		for (auto* Ptr = FocusedObj->GetParent().Get(); Ptr; Ptr = Ptr->GetParent().Get())
		{
			if (Ptr == DisplayObject.Get())
				return true;
		}
	}

	return false;
}

void UGObject::RequestFocus()
{
	if (!DisplayObject.IsValid())
		return;

	TSharedPtr<SFGUICanvas> Canvas = GetUIRoot()->GetCanvas();
	if (Canvas.IsValid())
		Canvas->SetFocus(DisplayObject.Get());
}

int32 UGObject::GetSortingOrder() const
{
	return SortingOrder;
}

UGGroup* UGObject::GetGroup() const
{
	return Group.Get();
}

const FText& UGObject::GetTooltips() const
{
	return Tooltips;
}

bool UGObject::IsDraggable() const
{
	return bDraggable;
}

UGComponent* UGObject::GetParent() const
{
	return Parent.Get();
}

UGTreeNode* UGObject::GetTreeNode() const
{
	return TreeNode;
}

UGObject* UGObject::GetDraggingObject()
{
	return DraggingObject.Get();
}

void UGObject::HandlePositionChanged()
{
	DisplayObject->SetPosition(bPivotAsAnchor ? (Position - Size * Pivot) : Position);
}

void UGObject::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !bIsInitialized)
	{
		CreateDisplayObject();
		bIsInitialized = true;
	}
}

void UGObject::CreateDisplayObject() {}

void UGObject::HandleSizeChanged()
{
	DisplayObject->SetSize(Size);
}

void UGObject::HandleAlphaChanged()
{
	DisplayObject->SetRenderOpacity(Alpha);
}

void UGObject::HandleGrayedChanged()
{
	DisplayObject->SetSaturation(bGrayed ? 0.0f : 1.0f);
}

void UGObject::HandleVisibleChanged()
{
	DisplayObject->SetVisible(InternalVisible2());
}

void UGObject::HandleBlendModeChanged()
{
	DisplayObject->SetBlendMode(BlendMode);
}

void UGObject::HandleControllerChanged(UGController* Controller)
{
	bHandlingController = true;
	for (int32 i = 0; i < 10; i++)
	{
		const TSharedPtr<FGearBase>& Gear = Gears[i];
		if (Gear.IsValid() && Gear->GetController() == Controller)
			Gear->Apply();
	}
	bHandlingController = false;

	CheckGearDisplay();
}

void UGObject::SetupBeforeAdd(FByteBuffer* Buffer, int32 BeginPos)
{
	Buffer->Seek(BeginPos, 0);
	Buffer->Skip(5);

	ID = Buffer->ReadS();
	Name = Buffer->ReadS();
	float f1 = Buffer->ReadInt();
	float f2 = Buffer->ReadInt();
	SetPosition(FVector2D(f1, f2));

	if (Buffer->ReadBool())
	{
		InitSize.X = Buffer->ReadInt();
		InitSize.Y = Buffer->ReadInt();
		SetSize(InitSize, true);
	}

	if (Buffer->ReadBool())
	{
		MinSize.X = Buffer->ReadInt();
		MaxSize.X = Buffer->ReadInt();
		MinSize.Y = Buffer->ReadInt();
		MaxSize.Y = Buffer->ReadInt();
	}

	if (Buffer->ReadBool())
	{
		f1 = Buffer->ReadFloat();
		f2 = Buffer->ReadFloat();
		SetScale(FVector2D(f1, f2));
	}

	if (Buffer->ReadBool())
	{
		f1 = Buffer->ReadFloat();
		f2 = Buffer->ReadFloat();
		SetSkew(FVector2D(f1, f2));
	}

	if (Buffer->ReadBool())
	{
		f1 = Buffer->ReadFloat();
		f2 = Buffer->ReadFloat();
		SetPivot(FVector2D(f1, f2), Buffer->ReadBool());
	}

	f1 = Buffer->ReadFloat();
	if (f1 != 1)
		SetAlpha(f1);

	f1 = Buffer->ReadFloat();
	if (f1 != 0)
		SetRotation(f1);

	if (!Buffer->ReadBool())
		SetVisible(false);
	if (!Buffer->ReadBool())
		SetTouchable(false);
	if (Buffer->ReadBool())
		SetGrayed(true);
	SetBlendMode((EFairyBlendMode)Buffer->ReadByte());
	Buffer->ReadByte(); // filter

	const FString& str = Buffer->ReadS();
	if (!str.IsEmpty())
		UserData = str;
}

void UGObject::SetupAfterAdd(FByteBuffer* Buffer, int32 BeginPos)
{
	Buffer->Seek(BeginPos, 1);

	auto& str = Buffer->ReadS();
	if (!str.IsEmpty())
	{
		FString Key = GetLocalizationKey(ID, UserData.AsString());
		auto	txt = ToLocText(*GetPackageName(), *FString::Printf(TEXT("%s_tooltips"), *Key), *str);
		SetTooltips(txt);
	}

	int16 groupId = Buffer->ReadShort();
	if (groupId >= 0)
		Group = Cast<UGGroup>(Parent->GetChildAt(groupId));

	Buffer->Seek(BeginPos, 2);

	int16 cnt = Buffer->ReadShort();
	for (int32 i = 0; i < cnt; i++)
	{
		int16 nextPos = Buffer->ReadShort();
		nextPos += Buffer->GetPos();

		GetGear(Buffer->ReadByte())->Setup(Buffer);

		Buffer->SetPos(nextPos);
	}
}

void UGObject::InitDrag()
{
	if (bDraggable)
	{
		OnTouchBegin.AddUniqueDynamic(this, &UGObject::OnTouchBeginHandler);
		OnTouchMove.AddUniqueDynamic(this, &UGObject::OnTouchMoveHandler);
		OnTouchEnd.AddUniqueDynamic(this, &UGObject::OnTouchEndHandler);
	}
	else
	{
		OnTouchBegin.RemoveDynamic(this, &UGObject::OnTouchBeginHandler);
		OnTouchMove.RemoveDynamic(this, &UGObject::OnTouchMoveHandler);
		OnTouchEnd.RemoveDynamic(this, &UGObject::OnTouchEndHandler);
	}
}

void UGObject::DragBegin(int32 UserIndex, int32 PointerIndex)
{
	if (DispatchEvent(FUIEvents::DragStart))
		return;

	if (DraggingObject.IsValid())
	{
		UGObject* tmp = DraggingObject.Get();
		DraggingObject->StopDrag();
		DraggingObject.Reset();
		tmp->DispatchEvent(FUIEvents::DragEnd);
	}

	GlobalDragStart = GetApp()->GetTouchPosition(UserIndex, PointerIndex);
	GlobalRect = LocalToGlobalRect(FBox2D(FVector2D::ZeroVector, Size));
	DraggingObject = this;
	bDragTesting = false;

	GetApp()->AddMouseCaptor(UserIndex, PointerIndex, this);

	OnTouchMove.AddUniqueDynamic(this, &UGObject::OnTouchMoveHandler);
	OnTouchEnd.AddUniqueDynamic(this, &UGObject::OnTouchEndHandler);
}

void UGObject::DragEnd()
{
	if (DraggingObject.Get() == this)
	{
		bDragTesting = false;
		DraggingObject.Reset();
	}
}

void UGObject::OnTouchBeginHandler(UEventContext* Context)
{
	DragTouchStartPos = Context->GetPointerPosition();
	bDragTesting = true;
	Context->CaptureTouch();
}

void UGObject::OnTouchMoveHandler(UEventContext* Context)
{
	if (DraggingObject.Get() != this && bDragTesting)
	{
		int32 sensitivity;
		if (FPlatformMisc::DesktopTouchScreen())
			sensitivity = FUIConfig::Config.ClickDragSensitivity;
		else
			sensitivity = FUIConfig::Config.TouchDragSensitivity;
		if (FMath::Abs(DragTouchStartPos.X - Context->GetPointerPosition().X) < sensitivity
			&& FMath::Abs(DragTouchStartPos.Y - Context->GetPointerPosition().Y) < sensitivity)
			return;

		bDragTesting = false;
		DragBegin(Context->GetUserIndex(), Context->GetPointerIndex());
	}

	if (DraggingObject.Get() == this)
	{
		FVector2D Pos = Context->GetPointerPosition() - GlobalDragStart + GlobalRect.Min;
		if (DragBounds.IsSet())
		{
			FBox2D rect = GetUIRoot()->LocalToGlobalRect(DragBounds.GetValue());
			if (Pos.X < rect.Min.X)
				Pos.X = rect.Min.X;
			else if (Pos.X + GlobalRect.GetSize().X > rect.Max.X)
			{
				Pos.X = rect.Max.X - GlobalRect.GetSize().X;
				if (Pos.X < rect.Min.X)
					Pos.X = rect.Min.X;
			}

			if (Pos.Y < rect.Min.Y)
				Pos.Y = rect.Min.Y;
			else if (Pos.Y + GlobalRect.GetSize().Y > rect.Max.Y)
			{
				Pos.Y = rect.Max.Y - GlobalRect.GetSize().Y;
				if (Pos.Y < rect.Min.Y)
					Pos.Y = rect.Min.Y;
			}
		}

		Pos = Parent->GlobalToLocalInContainer(Pos);

		bUpdateInDragging = true;
		SetPosition(Pos.RoundToVector());
		bUpdateInDragging = false;

		DispatchEvent(FUIEvents::DragMove);
	}
}

void UGObject::OnTouchEndHandler(UEventContext* Context)
{
	if (DraggingObject.Get() == this)
	{
		DraggingObject.Reset();
		DispatchEvent(FUIEvents::DragEnd);
	}
}