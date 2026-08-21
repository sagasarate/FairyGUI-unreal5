#include "Widgets/Renderer/RenderGroup.h"
#include "UI/UIConfig.h"
#include "FairyCommons.h"

void FRenderGroup::DumpRenderTree(int32 Indent) const
{
	TStringBuilder<256> IndentStr;
	for (int32 i = 0; i < Indent; ++i)
		IndentStr.Append(TEXT("  "));

	auto WorldBoundsStr = [](const FBox2D& B) -> FString {
		if (!B.bIsValid)
			return TEXT("(invalid)");
		return FString::Printf(TEXT("(%.0f,%.0f)-(%.0f,%.0f)"), B.Min.X, B.Min.Y, B.Max.X, B.Max.Y);
	};

	UE_LOG(LogFairyGUI, Log, TEXT("%s[Group] %s  ChildCount=%d  bClip=%s  StencilRef=%d"), *IndentStr,
		*WorldBoundsStr(WorldBounds), Children.Num(), bHasClipping ? TEXT("Y") : TEXT("N"), StencilRef);

	for (auto pChild : Children)
	{
		if (pChild->Type == ERenderUnitType::Group)
		{
			(static_cast<const FRenderGroup*>(pChild))->DumpRenderTree(Indent + 1);
		}
		else
		{
			const FRenderUnit* pUnit = static_cast<const FRenderUnit*>(pChild);
			const TCHAR*	   TypeName = TEXT("?");
			switch (pUnit->RendererType)
			{
				case EFairyRendererType::Textured:
					TypeName = TEXT("Unit[Textured]");
					break;
				case EFairyRendererType::Text:
					TypeName = TEXT("Unit[Text]");
					break;
				case EFairyRendererType::NoTexture:
					TypeName = TEXT("Unit[NoTexture]");
					break;
			}
			UE_LOG(LogFairyGUI, Log, TEXT("%s  %s  Verts=%d  Tris=%d  %s"), *IndentStr, TypeName, pUnit->Vertices.Num(),
				pUnit->Triangles.Num(), *WorldBoundsStr(pUnit->WorldBounds));
		}
	}
}

void FRenderGroup::SortRenderUnits()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FRenderGroup_SortRenderUnits);
	// 递归子容器（所有组都需要递归）
	for (auto pChild : Children)
	{
		if (pChild->Type == ERenderUnitType::Group)
			((FRenderGroup*)pChild)->SortRenderUnits();
	}

	// 对 Children 中所有子节点统一按 AABB 安全排序（Unit + 普通组 + 裁剪组）
	// 遮罩组：跳过前 MaskChildCount 个遮罩单元，其余全部参与排序
	int32 StartIdx = bHasMask ? MaskChildCount : 0;
	SortChildrenUnits(StartIdx);
}

void FRenderGroup::SortChildrenUnits(int32 StartIdx)
{
	int32 Count = Children.Num() - StartIdx;
	if (Count <= 1)
		return;

	// --- 层号计算：AABB 重叠检测，层号直接写入 FRenderUnit::Layer ---
	auto AABBOverlap = [](const FBox2D& A, const FBox2D& B) -> bool {
		constexpr float Eps = 1e-3f;	// 0.001 容差消除浮点累积误差
		return (A.Max.X > B.Min.X + Eps && B.Max.X > A.Min.X + Eps &&
				A.Max.Y > B.Min.Y + Eps && B.Max.Y > A.Min.Y + Eps);
	};

	for (int32 i = 0; i < Count; ++i)
	{
		int32		  MaxOverlapLayer = -1;
		const FBox2D& BoundsI = Children[StartIdx + i]->WorldBounds;
		for (int32 j = 0; j < i; ++j)
		{
			if (AABBOverlap(BoundsI, Children[StartIdx + j]->WorldBounds))
				MaxOverlapLayer = FMath::Max(MaxOverlapLayer, Children[StartIdx + j]->Layer);
		}
		Children[StartIdx + i]->Layer = MaxOverlapLayer + 1;
	}

	// --- 直接在 Children 切片上稳定排序，零临时数组 ---
	TArrayView<FRenderUnit*> View(Children.GetData() + StartIdx, Count);
	View.StableSort([](const FRenderUnit& A, const FRenderUnit& B) {
		if (A.Layer != B.Layer)
			return A.Layer < B.Layer;
		if (A.Type != B.Type)
			return A.Type < B.Type;
		if (A.RendererType != B.RendererType)
			return A.RendererType < B.RendererType;
		if (A.BlendMode != B.BlendMode)
			return A.BlendMode < B.BlendMode;
		if ((int32)A.TextDrawType / 3 != (int32)B.TextDrawType / 3)
			return A.TextDrawType < B.TextDrawType;
		auto TexPtr = [](const FRenderUnit& U) -> const void* {
			switch (U.RendererType)
			{
				case EFairyRendererType::Textured: return U.Texture;
				case EFairyRendererType::Text:
					return U.Texture ? static_cast<const void*>(U.Texture) : static_cast<const void*>(U.FontTexture);
				default: return nullptr;
			}
		};
		return TexPtr(A) < TexPtr(B);
	});
}

CIDStorage<FRenderGroup> FRenderGroup::Pool;

FRenderGroup* FRenderGroup::Borrow()
{
	if (Pool.GetBufferSize() <= 0)
		Pool.Create(256, 256, FUIConfig::Config.RenderUnitPoolGrowLimit);
	FRenderGroup* pInfo = Pool.NewObject();
	if (pInfo == nullptr)
	{
		UE_LOG(
			LogFairyGUI, Error, TEXT("FRenderGroup pool is full(%u/%u)!"), Pool.GetObjectCount(), Pool.GetBufferSize());
	}
	return pInfo;
}
void FRenderGroup::Return(FRenderGroup* pValue)
{
	uint32 ID = pValue->PoolID;
	pValue->Clear();
	Pool.DeleteObject(ID);
}