#pragma once

#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "Textures/TextureAtlas.h"
#include "Widgets/Renderer/FairyShaderCommon.h"
#include "Widgets/Renderer/RenderGroup.h"

class FAIRYGUI_API FFairyRenderPipeline final : public ICustomSlateElement
{
protected:
	// 批次结构 — 只存元数据（偏移量/计数），不持有实际数据
	struct FRenderBatch
	{
		EFairyRendererType RendererType = EFairyRendererType::Textured;
		FRHITexture*	   TextureRHI = nullptr;
		uint8			   StencilRef = 0; // 子元素 stencil 测试参考值
		ETextDrawType	   TextDrawType = ETextDrawType::AlphaBitmap;
		EFairyBlendMode		   BlendMode = EFairyBlendMode::Normal;

		// === 裁剪（Scissor）===
		FIntRect ScissorRect; // 空矩形(0,0,0,0)=不裁剪; 非空=SetScissorRect

		// === 遮罩（Stencil）===
		bool  bIsMaskWrite = false;		// ColorMask=0, stencil write, alpha clip
		bool  bIsMaskClear = false;		// ColorMask=0, stencil clear
		bool  bStencilInverted = false; // 反转 stencil 测试 (CF_NotEqual 替代 CF_Equal)
		uint8 MaskWriteRef = 0;			// mask write/clear 的目标 stencil 值
		uint8 MaskTestRef = 0;			// mask write/clear 时的测试值 (0=CF_Always)

		// === 批处理元数据 ===
		int32 BaseVertex = 0;
		int32 NumVertices = 0;
		int32 FirstIndex = 0;
		int32 NumIndices = 0;
		int32 ObjectDataBase = 0;
		int32 NumObjectData = 0;

		bool CanMergeWith(const FRenderBatch& Other) const
		{
			return RendererType == Other.RendererType && TextureRHI == Other.TextureRHI
				&& StencilRef == Other.StencilRef && ScissorRect == Other.ScissorRect
				&& bIsMaskWrite == Other.bIsMaskWrite && bIsMaskClear == Other.bIsMaskClear
				&& bStencilInverted == Other.bStencilInverted && MaskWriteRef == Other.MaskWriteRef
				&& MaskTestRef == Other.MaskTestRef && TextDrawType / 3 == Other.TextDrawType / 3
				&& BlendMode == Other.BlendMode;
		}
	};

	// 渲染线程本地批次数据
	struct FRenderBatchs
	{
		TArray<FRenderBatch> Batches;
		TArray<FSlateVertex> AllVertices;
		TArray<SlateIndex>	 AllIndices;
		TArray<FObjectData>	 AllObjectData;

		void Reset()
		{
			Batches.Reset();
			AllVertices.Reset();
			AllIndices.Reset();
			AllObjectData.Reset();
		}
	};

	// FRenderGroup 传递队列
	TQueue<FRenderGroup*, EQueueMode::Spsc> m_PendingGroups;	// 游戏线程 → 渲染线程
	TQueue<FRenderGroup*, EQueueMode::Mpsc> m_ReleaseGroups;	// 渲染线程 → 游戏线程

	// 持有当前帧批次数据直到下次 Draw_RenderThread（QueueBufferUpload 异步安全）
	FRenderBatchs m_ActiveBatches;

public:
	FFairyRenderPipeline() = default;

	// 游戏线程：将收集好的 FRenderGroup 树入队
	void SetRootGroup(FRenderGroup* pGroup);

	// 游戏线程：回收渲染线程已处理完的 FRenderGroup
	void ReturnPendingGroups();

	// ICustomSlateElement —— 渲染线程
	virtual void Draw_RenderThread(FRDGBuilder& GraphBuilder, const FDrawPassInputs& Inputs) override;

protected:
	// 以下方法全部在渲染线程执行
	void BuildBatchList(const FRenderGroup* Group, FRenderBatchs& OutBatchs, uint8 CurrentStencilRef,
		FIntRect CurrentScissor, bool bReversedMask = false);
	void EmitRenderUnit(const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 StencilRef, FIntRect ScissorRect,
		bool bStencilInverted = false);
	void EmitMaskWriteBatch(
		const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 WriteRef, uint8 ParentTestRef, FIntRect ScissorRect);
	void EmitMaskClearBatch(
		const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 ClearRef, uint8 RestoreRef, FIntRect ScissorRect);
	void EmitMaskWriteRecursive(
		const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 WriteRef, uint8 ParentTestRef, FIntRect ScissorRect);
	void EmitMaskClearRecursive(
		const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 ClearRef, uint8 RestoreRef, FIntRect ScissorRect);
	void CopyUnitToBatch(const FRenderUnit* pUnit, FRenderBatch* pBatch, FRenderBatchs& OutBatchs);
};
