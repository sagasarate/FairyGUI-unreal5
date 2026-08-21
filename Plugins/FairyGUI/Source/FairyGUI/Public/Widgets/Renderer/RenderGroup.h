#pragma once

#include "CoreMinimal.h"
#include "RenderUnit.h"
#include "Utils/IDStorage.h"

struct FRenderGroup;

struct FRenderGroup : public FRenderUnit
{
public:
	FIntRect			 ClippingRect;
	uint8				 StencilRef = 0; // 模板引用值，0=无裁剪
	TArray<FRenderUnit*> Children;		 // 有序子节点列表（保持原始渲染顺序）
	bool				 bHasClipping = false;
	// mask 子节点：位于 Children 数组最前部，数量由 MaskChildCount 指定
	int32                MaskChildCount = 0;
	bool				 bHasMask = false;
	bool				 bReversedMask = false;

	FRenderGroup()
	{
		Type = ERenderUnitType::Group;
		Clear();
	}

	void Clear()
	{
		FRenderUnit::Clear();
		StencilRef = 0;
		// 初始化为极大矩形，确保 IsOutsize 在无裁剪组上不会错误剔除几何体
		constexpr int32 Huge = INT_MAX / 2;
		ClippingRect = FIntRect(FIntPoint(-Huge, -Huge), FIntPoint(Huge, Huge));
		for (auto pChild : Children)
		{
			if (pChild->Type == ERenderUnitType::Group)
				FRenderGroup::Return((FRenderGroup*)pChild);
			else
				FRenderUnit::Return((FRenderUnit*)pChild);
		}
		Children.Reset();
		MaskChildCount = 0;
		bHasMask = false;
		bReversedMask = false;
		bHasClipping = false;
	}

	bool AddChild(FRenderUnit* pUnit)
	{
		if (!pUnit)
			return false;
		if (!bHasClipping)
			WorldBounds += pUnit->WorldBounds;
		Children.Add(pUnit);
		return true;
	}

	void CollectAllGroups(TArray<FRenderGroup*>& OutGroups)
	{
		OutGroups.Add(this);
		for (auto pChild : Children)
		{
			if (pChild->Type == ERenderUnitType::Group)
				((FRenderGroup*)pChild)->CollectAllGroups(OutGroups);
		}
	}
	void DumpRenderTree(int32 Indent = 0) const;
	void SortRenderUnits();

	// 核心：整数判断，与管线 FIntRect 完全一致
	bool IsOutsize(const FIntRect& Rect) const
	{
		return Rect.Max.X <= ClippingRect.Min.X || Rect.Min.X >= ClippingRect.Max.X
			|| Rect.Max.Y <= ClippingRect.Min.Y || Rect.Min.Y >= ClippingRect.Max.Y;
	}

	// 浮点重载：统一按 Floor(L/T) / Ceil(R/B) 规则转整数后判断
	bool IsOutsize(const FSlateRect& Rect) const
	{
		return IsOutsize(FIntRect(
			FMath::FloorToInt(Rect.Left), FMath::FloorToInt(Rect.Top),
			FMath::CeilToInt(Rect.Right), FMath::CeilToInt(Rect.Bottom)));
	}

	bool IsOutsize(const FBox2D& WorldBox) const
	{
		return IsOutsize(FIntRect(
			FMath::FloorToInt(WorldBox.Min.X), FMath::FloorToInt(WorldBox.Min.Y),
			FMath::CeilToInt(WorldBox.Max.X), FMath::CeilToInt(WorldBox.Max.Y)));
	}

	bool IsOutsize(const FBox2D& LocalBox, const FMatrix44f& LocalToWorldMat) const
	{
		// 1. 获取本地空间的 4 个顶点
		FVector3f Corners[4];
		Corners[0] = FVector3f(LocalBox.Min.X, LocalBox.Min.Y, 0.0f);
		Corners[1] = FVector3f(LocalBox.Max.X, LocalBox.Min.Y, 0.0f);
		Corners[2] = FVector3f(LocalBox.Max.X, LocalBox.Max.Y, 0.0f);
		Corners[3] = FVector3f(LocalBox.Min.X, LocalBox.Max.Y, 0.0f);

		// 2. 变换顶点并直接计算包围盒的 Min/Max (避免多次循环)
		FVector3f TransformedFirst = LocalToWorldMat.TransformPosition(Corners[0]);
		float	  MinX = TransformedFirst.X;
		float	  MinY = TransformedFirst.Y;
		float	  MaxX = TransformedFirst.X;
		float	  MaxY = TransformedFirst.Y;

		for (int32 i = 1; i < 4; ++i)
		{
			FVector3f P = LocalToWorldMat.TransformPosition(Corners[i]);
			if (P.X < MinX) MinX = P.X;
			if (P.X > MaxX) MaxX = P.X;
			if (P.Y < MinY) MinY = P.Y;
			if (P.Y > MaxY) MaxY = P.Y;
		}

		// 3. 转整数后统一判断
		return IsOutsize(FIntRect(
			FMath::FloorToInt(MinX), FMath::FloorToInt(MinY),
			FMath::CeilToInt(MaxX), FMath::CeilToInt(MaxY)));
	}

protected:
	void SortChildrenUnits(int32 StartIdx = 0);

public:
	static CIDStorage<FRenderGroup> Pool;

	static FRenderGroup* Borrow();
	static void			 Return(FRenderGroup* pValue);
};
