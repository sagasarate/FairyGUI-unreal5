#include "Widgets/Renderer/FairyRenderPipeline.h"
#include "FairyCommons.h"
#include "Widgets/Renderer/FairyRenderShader.h"
#include "Widgets/Renderer/FairyFontShader.h"
#include "Widgets/Renderer/FairyFontSdfShader.h"

inline bool IsBlendModePMA(EFairyBlendMode Mode)
{
	return Mode == EFairyBlendMode::Multiply || Mode == EFairyBlendMode::Screen;
}
#include "Widgets/Renderer/FairyNoTextureRenderShader.h"
#include "SlateRHIFontTexture.h"
#include "RenderGraphBuilder.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "RHICommandList.h"

BEGIN_SHADER_PARAMETER_STRUCT(FFairyPipelinePassParams, )
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

void FFairyRenderPipeline::SetRootGroup(FRenderGroup* pGroup)
{
	// 仅入队，不排序不合批——排序合批移到渲染线程 BuildBatchList 之前
	m_PendingGroups.Enqueue(pGroup);
}

void FFairyRenderPipeline::ReturnPendingGroups()
{
	// 游戏线程调用，归还渲染线程已处理完的 FRenderGroup 到对象池
	FRenderGroup* pGroup;
	while (m_ReleaseGroups.Dequeue(pGroup))
	{
		FRenderGroup::Return(pGroup);
	}
}

void FFairyRenderPipeline::CopyUnitToBatch(const FRenderUnit* pUnit, FRenderBatch* pBatch, FRenderBatchs& OutBatchs)
{
	pBatch->BaseVertex = OutBatchs.AllVertices.Num();
	pBatch->NumVertices = pUnit->Vertices.Num();
	pBatch->FirstIndex = OutBatchs.AllIndices.Num();
	pBatch->NumIndices = pUnit->Triangles.Num();
	pBatch->ObjectDataBase = OutBatchs.AllObjectData.Num();
	pBatch->NumObjectData = 1;

	FObjectData& Obj = OutBatchs.AllObjectData.AddDefaulted_GetRef();
	Obj.LocalToWorld = pUnit->LocalToWorld;
	Obj.Alpha = pUnit->Alpha;
	Obj.Saturation = pUnit->Saturation;
	Obj.SdfBias = pUnit->SdfBias;
	Obj.SdfPixelSpread = pUnit->SdfPixelSpread;
	Obj.OutlineColor = pUnit->OutlineColor;
	Obj.OutlineBias = pUnit->OutlineBias;
	Obj.TextDrawType = static_cast<int>(pUnit->TextDrawType);
	Obj.ColorOption = IsBlendModePMA(pUnit->BlendMode) ? 1 : 0;

	const int32 ObjectIndex = pBatch->ObjectDataBase;
	for (const FSlateVertex& SV : pUnit->Vertices)
	{
		FSlateVertex NV = SV;
		FMemory::Memcpy(&NV.TexCoords[2], &ObjectIndex, sizeof(float));
		OutBatchs.AllVertices.Add(NV);
	}
	for (SlateIndex Idx : pUnit->Triangles)
		OutBatchs.AllIndices.Add(Idx);
}

void FFairyRenderPipeline::EmitRenderUnit(
	const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 StencilRef, FIntRect ScissorRect, bool bStencilInverted)
{
	if (pUnit->Vertices.IsEmpty() || pUnit->Triangles.IsEmpty())
		return;

	// 用索引替代指针：Emplace_GetRef 可能触发 TArray 重分配使 pPrev 悬垂
	const int32 PrevIndex = OutBatchs.Batches.Num() - 1;

	FRenderBatch& NewBatch = OutBatchs.Batches.Emplace_GetRef();
	// Emplace_GetRef 后重新安全获取指针（若发生 realloc，数组已迁移到新缓冲区）
	FRenderBatch* pPrev = (PrevIndex >= 0) ? &OutBatchs.Batches[PrevIndex] : nullptr;
	NewBatch.RendererType = pUnit->RendererType;
	NewBatch.StencilRef = StencilRef;
	NewBatch.ScissorRect = ScissorRect;
	NewBatch.bIsMaskWrite = false;
	NewBatch.bIsMaskClear = false;
	NewBatch.bStencilInverted = bStencilInverted;
	NewBatch.MaskWriteRef = 0;
	NewBatch.MaskTestRef = 0;
	NewBatch.TextDrawType = pUnit->TextDrawType;
	NewBatch.BlendMode = pUnit->BlendMode;

	if (pUnit->RendererType == EFairyRendererType::Textured)
	{
		if (!pUnit->Texture || !pUnit->Texture->TextureRHI)
		{
			OutBatchs.Batches.Pop();
			return;
		}
		NewBatch.TextureRHI = pUnit->Texture->TextureRHI.GetReference();
	}
	else if (pUnit->RendererType == EFairyRendererType::Text)
	{
		if (pUnit->FontTexture)
		{
			auto* TexRHI = static_cast<FSlateFontTextureRHIResource*>(pUnit->FontTexture);
			NewBatch.TextureRHI = TexRHI->GetTypedResource().GetReference();
		}
		else if (pUnit->Texture && pUnit->Texture->TextureRHI)
		{
			NewBatch.TextureRHI = pUnit->Texture->TextureRHI.GetReference();
		}
		else
		{
			OutBatchs.Batches.Pop();
			return;
		}
		if (!NewBatch.TextureRHI)
		{
			OutBatchs.Batches.Pop();
			return;
		}
	}

	// 尝试与上一个批次合并（同纹理+同状态才可合批）
	if (pPrev && pPrev->CanMergeWith(NewBatch))
	{
		const int32 ObjectIndex = OutBatchs.AllObjectData.Num();

		// ObjectData
		FObjectData& Obj = OutBatchs.AllObjectData.AddDefaulted_GetRef();
		Obj.LocalToWorld = pUnit->LocalToWorld;
		Obj.Alpha = pUnit->Alpha;
		Obj.Saturation = pUnit->Saturation;
		Obj.SdfBias = pUnit->SdfBias;
		Obj.SdfPixelSpread = pUnit->SdfPixelSpread;
		Obj.OutlineColor = pUnit->OutlineColor;
		Obj.OutlineBias = pUnit->OutlineBias;
		Obj.TextDrawType = static_cast<int>(pUnit->TextDrawType);
		Obj.ColorOption = IsBlendModePMA(pUnit->BlendMode) ? 1 : 0;

		// 顶点：设置全局 ObjectIndex
		for (const FSlateVertex& SV : pUnit->Vertices)
		{
			FSlateVertex NV = SV;
			FMemory::Memcpy(&NV.TexCoords[2], &ObjectIndex, sizeof(float));
			OutBatchs.AllVertices.Add(NV);
		}

		// 索引：偏移到合并后该 batch 内的顶点位置
		for (SlateIndex Idx : pUnit->Triangles)
			OutBatchs.AllIndices.Add(Idx + pPrev->NumVertices);

		// 更新前一 batch 计数
		pPrev->NumVertices += pUnit->Vertices.Num();
		pPrev->NumIndices += pUnit->Triangles.Num();
		pPrev->NumObjectData += 1;

		OutBatchs.Batches.Pop(); // 丢弃未使用的 NewBatch
	}
	else
	{
		CopyUnitToBatch(pUnit, &NewBatch, OutBatchs);
	}
}

void FFairyRenderPipeline::EmitMaskWriteBatch(
	const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 WriteRef, uint8 ParentTestRef, FIntRect ScissorRect)
{
	if (pUnit->Vertices.IsEmpty() || pUnit->Triangles.IsEmpty())
		return;

	FRenderBatch& Batch = OutBatchs.Batches.Emplace_GetRef();
	Batch.RendererType = pUnit->RendererType;
	Batch.StencilRef = WriteRef;
	Batch.ScissorRect = ScissorRect;
	Batch.bIsMaskWrite = true;
	Batch.MaskWriteRef = WriteRef;
	Batch.MaskTestRef = ParentTestRef;
	Batch.TextDrawType = pUnit->TextDrawType;

	if (pUnit->RendererType == EFairyRendererType::Textured)
	{
		if (!pUnit->Texture || !pUnit->Texture->TextureRHI)
		{
			OutBatchs.Batches.Pop();
			return;
		}
		Batch.TextureRHI = pUnit->Texture->TextureRHI.GetReference();
	}
	else if (pUnit->RendererType == EFairyRendererType::Text)
	{
		if (pUnit->FontTexture)
		{
			auto* TexRHI = static_cast<FSlateFontTextureRHIResource*>(pUnit->FontTexture);
			Batch.TextureRHI = TexRHI->GetTypedResource().GetReference();
		}
		else if (pUnit->Texture && pUnit->Texture->TextureRHI)
			Batch.TextureRHI = pUnit->Texture->TextureRHI.GetReference();
		else
		{
			OutBatchs.Batches.Pop();
			return;
		}
		if (!Batch.TextureRHI)
		{
			OutBatchs.Batches.Pop();
			return;
		}
	}

	CopyUnitToBatch(pUnit, &Batch, OutBatchs);
}

void FFairyRenderPipeline::EmitMaskClearBatch(
	const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 ClearRef, uint8 RestoreRef, FIntRect ScissorRect)
{
	if (pUnit->Vertices.IsEmpty() || pUnit->Triangles.IsEmpty())
		return;

	FRenderBatch& Batch = OutBatchs.Batches.Emplace_GetRef();
	Batch.RendererType = pUnit->RendererType;
	Batch.StencilRef = ClearRef;
	Batch.ScissorRect = ScissorRect;
	Batch.bIsMaskClear = true;
	Batch.MaskWriteRef = ClearRef;
	Batch.MaskTestRef = ClearRef;
	Batch.TextDrawType = pUnit->TextDrawType;

	if (pUnit->RendererType == EFairyRendererType::Textured)
	{
		if (!pUnit->Texture || !pUnit->Texture->TextureRHI)
		{
			OutBatchs.Batches.Pop();
			return;
		}
		Batch.TextureRHI = pUnit->Texture->TextureRHI.GetReference();
	}
	else if (pUnit->RendererType == EFairyRendererType::Text)
	{
		if (pUnit->FontTexture)
		{
			auto* TexRHI = static_cast<FSlateFontTextureRHIResource*>(pUnit->FontTexture);
			Batch.TextureRHI = TexRHI->GetTypedResource().GetReference();
		}
		else if (pUnit->Texture && pUnit->Texture->TextureRHI)
			Batch.TextureRHI = pUnit->Texture->TextureRHI.GetReference();
		else
		{
			OutBatchs.Batches.Pop();
			return;
		}
		if (!Batch.TextureRHI)
		{
			OutBatchs.Batches.Pop();
			return;
		}
	}

	CopyUnitToBatch(pUnit, &Batch, OutBatchs);
}

void FFairyRenderPipeline::EmitMaskWriteRecursive(
	const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 WriteRef, uint8 ParentTestRef, FIntRect ScissorRect)
{
	if (pUnit->Type == ERenderUnitType::Group)
	{
		const FRenderGroup* pGroup = static_cast<const FRenderGroup*>(pUnit);
		for (FRenderUnit* pChild : pGroup->Children)
			EmitMaskWriteRecursive(pChild, OutBatchs, WriteRef, ParentTestRef, ScissorRect);
	}
	else
	{
		EmitMaskWriteBatch(pUnit, OutBatchs, WriteRef, ParentTestRef, ScissorRect);
	}
}

void FFairyRenderPipeline::EmitMaskClearRecursive(
	const FRenderUnit* pUnit, FRenderBatchs& OutBatchs, uint8 ClearRef, uint8 RestoreRef, FIntRect ScissorRect)
{
	if (pUnit->Type == ERenderUnitType::Group)
	{
		const FRenderGroup* pGroup = static_cast<const FRenderGroup*>(pUnit);
		for (FRenderUnit* pChild : pGroup->Children)
			EmitMaskClearRecursive(pChild, OutBatchs, ClearRef, RestoreRef, ScissorRect);
	}
	else
	{
		EmitMaskClearBatch(pUnit, OutBatchs, ClearRef, RestoreRef, ScissorRect);
	}
}

void FFairyRenderPipeline::BuildBatchList(const FRenderGroup* Group, FRenderBatchs& OutBatchs, uint8 CurrentStencilRef,
	FIntRect CurrentScissor, bool bReversedMask)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FFairyRenderPipeline_BuildBatchList);
	uint8 ContentRef = CurrentStencilRef;
	bool  bEffReversed;
	bool  bIgnoreMask = false; // 父正向+子反向 → 完全跳过此层 mask
	// CurrentStencilRef>0: 确保父级确实有 mask 生效，避免平级反向 mask 被误判
	if (Group->bHasMask && Group->bReversedMask && !bReversedMask && CurrentStencilRef > 0)
	{
		bEffReversed = false;
		bIgnoreMask = true; // Unity: 子反向被忽略，mask write/clear 也跳过
	}
	else if (Group->bHasMask)
		bEffReversed = Group->bReversedMask;
	else
		bEffReversed = bReversedMask;
	FIntRect EffectiveScissor = CurrentScissor;

	// === 1. Scissor 裁剪（嵌套：与父级取交集）===
	if (Group->bHasClipping)
	{
		const FIntRect& GroupScissor = Group->ClippingRect;
		if (EffectiveScissor.IsEmpty())
			EffectiveScissor = GroupScissor;
		else
			EffectiveScissor = FIntRect(FMath::Max(EffectiveScissor.Min.X, GroupScissor.Min.X),
				FMath::Max(EffectiveScissor.Min.Y, GroupScissor.Min.Y),
				FMath::Min(EffectiveScissor.Max.X, GroupScissor.Max.X),
				FMath::Min(EffectiveScissor.Max.Y, GroupScissor.Max.Y));
	}

	// === 2. Mask Write（渲染 mask 子节点 → 写 stencil）===
	// 递增方式: L0 写 1, L1 写 2, L2 写 3...（与 SO_SaturatedIncrement 兼容）
	uint8 MaskWriteRef = 0;
	uint8 MaskParentTestRef = 0;
	if (Group->bHasMask && Group->MaskChildCount > 0 && !bIgnoreMask)
	{
		MaskParentTestRef = ContentRef; // 父级 stencil 值 (0=最外层)
		MaskWriteRef = ContentRef + 1;	// 当前层写入值
		ContentRef = MaskWriteRef;		// 子元素测试此值

		for (int32 i = 0; i < Group->MaskChildCount; i++)
		{
			EmitMaskWriteRecursive(Group->Children[i], OutBatchs, MaskWriteRef, MaskParentTestRef, EffectiveScissor);
		}
	}

	// === 3. Children（mask 子节点之后的正常子节点）===
	for (int32 i = Group->MaskChildCount; i < Group->Children.Num(); i++)
	{
		FRenderUnit* pChild = Group->Children[i];
		if (pChild->Type == ERenderUnitType::Unit)
			EmitRenderUnit(pChild, OutBatchs, ContentRef, EffectiveScissor, bEffReversed);
		else
			BuildBatchList((const FRenderGroup*)pChild, OutBatchs, ContentRef, EffectiveScissor, bEffReversed);
	}

	// === 4. Mask Clear（清除 stencil）===
	if (Group->bHasMask && Group->MaskChildCount > 0 && !bIgnoreMask)
	{
		for (int32 i = 0; i < Group->MaskChildCount; i++)
		{
			EmitMaskClearRecursive(Group->Children[i], OutBatchs, MaskWriteRef, MaskParentTestRef, EffectiveScissor);
		}
	}
}

void FFairyRenderPipeline::Draw_RenderThread(FRDGBuilder& GraphBuilder, const FDrawPassInputs& Inputs)
{
	// 1. 取出待渲染的 FRenderGroup 树
	FRenderGroup* pGroup;
	if (!m_PendingGroups.Dequeue(pGroup))
		return;

	// 2. 复用 m_ActiveBatches（Reset 保留已分配内存）
	m_ActiveBatches.Reset();

	// 3. 排序 + 合批（全部在渲染线程执行）
	pGroup->SortRenderUnits();
	const FIntRect& ViewportRect = pGroup->ClippingRect;
	BuildBatchList(pGroup, m_ActiveBatches, 0, ViewportRect);

	if (m_ActiveBatches.AllVertices.Num() == 0)
	{
		m_ReleaseGroups.Enqueue(pGroup);
		return;
	}
#ifdef RENDER_DIAG
	// Diagnostic: validate batch data consistency (check indices, vertex bounds, ObjectData indices)
	{
		const int32	 TV = m_ActiveBatches.AllVertices.Num();
		const int32	 TI = m_ActiveBatches.AllIndices.Num();
		const int32	 TO = m_ActiveBatches.AllObjectData.Num();
		static int32 s_DiagFrame = 0;
		s_DiagFrame++;
		bool bBad = false;

		for (const FRenderBatch& B : m_ActiveBatches.Batches)
		{
			if (B.NumVertices <= 0 || B.NumIndices <= 0)
				continue;
			if (B.FirstIndex < 0 || B.FirstIndex + B.NumIndices > TI)
			{
				UE_LOG(LogFairyGUI, Error, TEXT("DIAG F%d: BAD IDX RANGE First=%d + Num=%d > Total=%d"),
					s_DiagFrame, B.FirstIndex, B.NumIndices, TI);
				bBad = true;
				continue;
			}
			if (B.BaseVertex < 0 || B.BaseVertex + B.NumVertices > TV)
			{
				UE_LOG(LogFairyGUI, Error, TEXT("DIAG F%d: BAD VTX RANGE Base=%d + Num=%d > Total=%d"),
					s_DiagFrame, B.BaseVertex, B.NumVertices, TV);
				bBad = true;
				continue;
			}
			if (B.ObjectDataBase < 0 || B.ObjectDataBase + B.NumObjectData > TO)
			{
				UE_LOG(LogFairyGUI, Error, TEXT("DIAG F%d: BAD OBJ RANGE Base=%d + Num=%d > Total=%d"),
					s_DiagFrame, B.ObjectDataBase, B.NumObjectData, TO);
				bBad = true;
			}
			// Check each index value is in range
			for (int32 i = B.FirstIndex; i < B.FirstIndex + B.NumIndices; i++)
			{
				SlateIndex Idx = m_ActiveBatches.AllIndices[i];
				if ((int32)Idx >= B.NumVertices)
				{
					UE_LOG(LogFairyGUI, Error, TEXT("DIAG F%d: IDX OOB IB[%d]=%d >= NumVerts=%d"),
						s_DiagFrame, i, (int32)Idx, B.NumVertices);
					bBad = true;
					break;
				}
			}
			// Check ObjectIndex embedded in vertices is in range
			if (!bBad)
			{
				for (int32 v = B.BaseVertex; v < B.BaseVertex + B.NumVertices; v++)
				{
					int32 OI;
					FMemory::Memcpy(&OI, &m_ActiveBatches.AllVertices[v].TexCoords[2], sizeof(int32));
					if (OI < 0 || OI >= TO)
					{
						UE_LOG(LogFairyGUI, Error, TEXT("DIAG F%d: OBJIDX OOB VB[%d].Tex[2]=%d >= TotalObj=%d"),
							s_DiagFrame, v, OI, TO);
						bBad = true;
						break;
					}
				}
			}
		}
		if (!bBad && s_DiagFrame % 120 == 1)
		{
			UE_LOG(LogFairyGUI, Log, TEXT("DIAG F%d OK: V=%d I=%d O=%d B=%d"),
				s_DiagFrame, TV, TI, TO, m_ActiveBatches.Batches.Num());
		}
	}
#endif

	// 5. 预取 shader
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	if (!ShaderMap)
	{
		m_ReleaseGroups.Enqueue(pGroup);
		return;
	}

	TShaderMapRef<FFairyRenderVS>		   TexturedVS(ShaderMap);
	TShaderMapRef<FFairyRenderPS>		   TexturedPS(ShaderMap);
	TShaderMapRef<FFairyFontVS>			   FontVS(ShaderMap);
	TShaderMapRef<FFairyFontPS>			   FontPS(ShaderMap);
	TShaderMapRef<FFairyFontSdfVS>		   FontSdfVS(ShaderMap);
	TShaderMapRef<FFairyFontSdfPS>		   FontSdfPS(ShaderMap);
	TShaderMapRef<FFairyNoTextureRenderVS> NoTexVS(ShaderMap);
	TShaderMapRef<FFairyNoTextureRenderPS> NoTexPS(ShaderMap);

	const FMatrix44f& ViewProjection = Inputs.ElementsMatrix;
	const FVector2f&  ViewOffset = Inputs.ElementsOffset;

	const int32 TotalVertices = m_ActiveBatches.AllVertices.Num();
	const int32 TotalIndices = m_ActiveBatches.AllIndices.Num();
	const int32 TotalObjectData = m_ActiveBatches.AllObjectData.Num();

	// 5. 创建全局 RDG 缓冲区
	FRDGBufferRef GlobalVB = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateBufferDesc(sizeof(FSlateVertex), TotalVertices), TEXT("FairyGlobalVB"));
	FRDGBufferRef GlobalIB = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateBufferDesc(sizeof(SlateIndex), TotalIndices), TEXT("FairyGlobalIB"));
	FRDGBufferRef GlobalObjBuf = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(sizeof(FObjectData), TotalObjectData), TEXT("FairyGlobalObjBuf"));

	// 6. 上传统一存储——m_ActiveBatches 持有数据直到下次 Draw_RenderThread
	GraphBuilder.QueueBufferUpload(GlobalVB, m_ActiveBatches.AllVertices.GetData(),
		TotalVertices * sizeof(FSlateVertex), ERDGInitialDataFlags::None);
	GraphBuilder.QueueBufferUpload(
		GlobalIB, m_ActiveBatches.AllIndices.GetData(), TotalIndices * sizeof(SlateIndex), ERDGInitialDataFlags::None);
	GraphBuilder.QueueBufferUpload(GlobalObjBuf, m_ActiveBatches.AllObjectData.GetData(),
		TotalObjectData * sizeof(FObjectData), ERDGInitialDataFlags::None);

	// 7. FRenderGroup 已处理完，入队待游戏线程回收
	m_ReleaseGroups.Enqueue(pGroup);

	// 8. 收集有效批次的元数据拷贝（FRenderBatch 仅含元数据，拷贝开销极小）
	TArray<FRenderBatch> BatchCopies;
	BatchCopies.Reserve(m_ActiveBatches.Batches.Num());
	for (FRenderBatch& Batch : m_ActiveBatches.Batches)
	{
		if (Batch.NumVertices > 0)
			BatchCopies.Add(Batch);
	}

	// 9. 创建 DS 纹理用于模板测试
	FIntPoint		TexSize = Inputs.OutputTexture->Desc.Extent;
	FRDGTextureDesc DSDesc = FRDGTextureDesc::Create2D(TexSize, PF_DepthStencil, FClearValueBinding::DepthFar,
		TexCreate_DepthStencilTargetable | TexCreate_ShaderResource);
	FRDGTextureRef	DSTexture = GraphBuilder.CreateTexture(DSDesc, TEXT("FairyStencilDS"));

	// 12. 提交 RDG Pass
	FFairyPipelinePassParams* PassParams = GraphBuilder.AllocParameters<FFairyPipelinePassParams>();
	PassParams->RenderTargets[0] = FRenderTargetBinding(Inputs.OutputTexture, ERenderTargetLoadAction::ELoad);
	PassParams->RenderTargets.DepthStencil = FDepthStencilBinding(DSTexture, ERenderTargetLoadAction::EClear,
		ERenderTargetLoadAction::EClear, FExclusiveDepthStencil::DepthNop_StencilWrite);

	GraphBuilder.AddPass(RDG_EVENT_NAME("FairyRenderPipeline"), PassParams, ERDGPassFlags::Raster,
		[BatchCopies = MoveTemp(BatchCopies), GlobalVB, GlobalIB, GlobalObjBuf, TexturedVS, TexturedPS, FontVS, FontPS,
			FontSdfVS, FontSdfPS, NoTexVS, NoTexPS, ViewProjection, ViewOffset, TexSize](FRHICommandList& RHICmdList) mutable {
			// Blend States
			static FRHIBlendState* NoColorBlendState = TStaticBlendState<CW_NONE>::GetRHI();
			static FRHIBlendState* NormalBlendStates[] = {
				TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI(), // 0:Normal
				TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One>::GetRHI(),						 // 1:None
				TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_One>::GetRHI(),				 // 2:Add
				TStaticBlendState<CW_RGBA, BO_Add, BF_DestColor, BF_InverseSourceAlpha>::GetRHI(),	 // 3:Multiply
				TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseDestColor>::GetRHI(),			 // 4:Screen
				TStaticBlendState<CW_RGBA, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI(),		 // 5:Erase
				TStaticBlendState<CW_RGBA, BO_Add, BF_Zero, BF_SourceAlpha>::GetRHI(),				 // 6:Mask
				TStaticBlendState<CW_RGBA, BO_Add, BF_InverseDestAlpha, BF_DestAlpha>::GetRHI(),	 // 7:Below
				TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_Zero>::GetRHI(),						 // 8:Off
				TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI(),		 // 9:One_OneMinusSrcAlpha
			};

			// DepthStencil States
			static FRHIDepthStencilState* NoStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			static FRHIDepthStencilState* MaskWriteState = TStaticDepthStencilState<false, CF_Always, true, CF_Always,
				SO_Keep, SO_Keep, SO_SaturatedIncrement>::GetRHI();
			static FRHIDepthStencilState* MaskWriteNestedState = TStaticDepthStencilState<false, CF_Always, true,
				CF_Equal, SO_Keep, SO_Keep, SO_SaturatedIncrement>::GetRHI();
			static FRHIDepthStencilState* MaskClearState =
				TStaticDepthStencilState<false, CF_Always, true, CF_Equal, SO_Keep, SO_Keep, SO_Decrement>::GetRHI();
			static FRHIDepthStencilState* StencilTestState =
				TStaticDepthStencilState<false, CF_Always, true, CF_Equal, SO_Keep, SO_Keep, SO_Keep>::GetRHI();
			static FRHIDepthStencilState* StencilTestInvertedState =
				TStaticDepthStencilState<false, CF_Always, true, CF_NotEqual, SO_Keep, SO_Keep, SO_Keep>::GetRHI();

			FShaderResourceViewRHIRef ObjSRV = RHICmdList.CreateShaderResourceView(
				GlobalObjBuf->GetRHI(), FRHIViewDesc::CreateBufferSRV().SetTypeFromBuffer(GlobalObjBuf->GetRHI()));

			FIntRect LastScissor;
			bool	 bScissorEnabled = false;

			for (const FRenderBatch& Batch : BatchCopies)
			{
				if (Batch.NumVertices == 0)
					continue;

				// === Scissor 管理（含防御性 clamp，防止 D3D12 越界异常）===
				bool bNeedScissor = !Batch.ScissorRect.IsEmpty();
				if (bNeedScissor)
				{
					FIntRect ClampedScissor = Batch.ScissorRect;
					ClampedScissor.Min.X = FMath::Clamp(ClampedScissor.Min.X, 0, TexSize.X);
					ClampedScissor.Min.Y = FMath::Clamp(ClampedScissor.Min.Y, 0, TexSize.Y);
					ClampedScissor.Max.X = FMath::Clamp(ClampedScissor.Max.X, 0, TexSize.X);
					ClampedScissor.Max.Y = FMath::Clamp(ClampedScissor.Max.Y, 0, TexSize.Y);
					if (!bScissorEnabled || ClampedScissor != LastScissor)
					{
						RHICmdList.SetScissorRect(true, ClampedScissor.Min.X, ClampedScissor.Min.Y,
							ClampedScissor.Max.X, ClampedScissor.Max.Y);
						bScissorEnabled = true;
						LastScissor = ClampedScissor;
					}
				}
				else if (bScissorEnabled)
				{
					RHICmdList.SetScissorRect(false, 0, 0, 0, 0);
					bScissorEnabled = false;
				}

				// === Pipeline State ===
				FGraphicsPipelineStateInitializer PipelineInit;
				PipelineInit.RasterizerState = TStaticRasterizerState<FM_Solid>::GetRHI();
				PipelineInit.PrimitiveType = PT_TriangleList;

				if (Batch.bIsMaskWrite)
				{
					PipelineInit.BlendState = NoColorBlendState;
					PipelineInit.DepthStencilState = (Batch.MaskTestRef == 0) ? MaskWriteState : MaskWriteNestedState;
				}
				else if (Batch.bIsMaskClear)
				{
					PipelineInit.BlendState = NoColorBlendState;
					PipelineInit.DepthStencilState = MaskClearState;
				}
				else
				{
					uint8 BlendIdx = static_cast<uint8>(Batch.BlendMode);
					if (BlendIdx >= UE_ARRAY_COUNT(NormalBlendStates))
						BlendIdx = 0;
					PipelineInit.BlendState = NormalBlendStates[BlendIdx];
					if (Batch.StencilRef > 0)
						PipelineInit.DepthStencilState =
							Batch.bStencilInverted ? StencilTestInvertedState : StencilTestState;
					else
						PipelineInit.DepthStencilState = NoStencilState;
				}

				// === Shader 绑定 ===
				switch (Batch.RendererType)
				{
					case EFairyRendererType::Textured:
						PipelineInit.BoundShaderState.VertexShaderRHI = TexturedVS.GetVertexShader();
						PipelineInit.BoundShaderState.PixelShaderRHI = TexturedPS.GetPixelShader();
						PipelineInit.BoundShaderState.VertexDeclarationRHI =
							GFairyRenderVertexDeclaration.VertexDeclarationRHI;
						break;
					case EFairyRendererType::Text:
						if (Batch.TextDrawType == ETextDrawType::AlphaBitmap
							|| Batch.TextDrawType == ETextDrawType::ColorBitmap
							|| Batch.TextDrawType == ETextDrawType::Line)
						{
							PipelineInit.BoundShaderState.VertexShaderRHI = FontVS.GetVertexShader();
							PipelineInit.BoundShaderState.PixelShaderRHI = FontPS.GetPixelShader();
						}
						else
						{
							PipelineInit.BoundShaderState.VertexShaderRHI = FontSdfVS.GetVertexShader();
							PipelineInit.BoundShaderState.PixelShaderRHI = FontSdfPS.GetPixelShader();
						}
						PipelineInit.BoundShaderState.VertexDeclarationRHI =
							GFairyFontVertexDeclaration.VertexDeclarationRHI;
						break;
					case EFairyRendererType::NoTexture:
						PipelineInit.BoundShaderState.VertexShaderRHI = NoTexVS.GetVertexShader();
						PipelineInit.BoundShaderState.PixelShaderRHI = NoTexPS.GetPixelShader();
						PipelineInit.BoundShaderState.VertexDeclarationRHI =
							GFairyRenderVertexDeclaration.VertexDeclarationRHI;
						break;
				}

				RHICmdList.ApplyCachedRenderTargets(PipelineInit);
				SetGraphicsPipelineState(RHICmdList, PipelineInit, 0, EApplyRendertargetOption::DoNothing, true);

				// === StencilRef ===
				if (Batch.bIsMaskWrite)
					RHICmdList.SetStencilRef(Batch.MaskTestRef);
				else if (Batch.bIsMaskClear)
					RHICmdList.SetStencilRef(Batch.StencilRef);
				else if (Batch.StencilRef > 0)
					RHICmdList.SetStencilRef(Batch.StencilRef);

				float AlphaMaskThreshold = Batch.bIsMaskWrite ? 0.001f : 0.0f;

				// === VS + PS 参数 ===
				switch (Batch.RendererType)
				{
					case EFairyRendererType::Textured:
					{
						FFairyRenderVS::FParameters P;
						P.ViewProjectionMatrix = ViewProjection;
						P.ViewOffset = ViewOffset;
						P.ObjectDataBuffer = ObjSRV;
						FRHIBatchedShaderParameters& B = RHICmdList.GetScratchShaderParameters();
						SetShaderParameters(B, TexturedVS, P);
						RHICmdList.SetBatchedShaderParameters(TexturedVS.GetVertexShader(), B);

						FFairyRenderPS::FParameters PP;
						PP.RenderTexture = Batch.TextureRHI;
						PP.RenderTextureSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
						PP.AlphaMaskThreshold = AlphaMaskThreshold;
						FRHIBatchedShaderParameters& BP = RHICmdList.GetScratchShaderParameters();
						SetShaderParameters(BP, TexturedPS, PP);
						RHICmdList.SetBatchedShaderParameters(TexturedPS.GetPixelShader(), BP);
					}
					break;
					case EFairyRendererType::Text:
					{
						if (Batch.TextDrawType == ETextDrawType::AlphaBitmap
							|| Batch.TextDrawType == ETextDrawType::ColorBitmap
							|| Batch.TextDrawType == ETextDrawType::Line)
						{
							FFairyFontVS::FParameters P;
							P.ViewProjectionMatrix = ViewProjection;
							P.ViewOffset = ViewOffset;
							P.ObjectDataBuffer = ObjSRV;
							FRHIBatchedShaderParameters& B = RHICmdList.GetScratchShaderParameters();
							SetShaderParameters(B, FontVS, P);
							RHICmdList.SetBatchedShaderParameters(FontVS.GetVertexShader(), B);

							FFairyFontPS::FParameters PP;
							PP.FontTexture = Batch.TextureRHI;
							PP.FontTextureSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
							PP.AlphaMaskThreshold = AlphaMaskThreshold;
							FRHIBatchedShaderParameters& BP = RHICmdList.GetScratchShaderParameters();
							SetShaderParameters(BP, FontPS, PP);
							RHICmdList.SetBatchedShaderParameters(FontPS.GetPixelShader(), BP);
						}
						else
						{
							FFairyFontSdfVS::FParameters P;
							P.ViewProjectionMatrix = ViewProjection;
							P.ViewOffset = ViewOffset;
							P.ObjectDataBuffer = ObjSRV;
							FRHIBatchedShaderParameters& B = RHICmdList.GetScratchShaderParameters();
							SetShaderParameters(B, FontSdfVS, P);
							RHICmdList.SetBatchedShaderParameters(FontSdfVS.GetVertexShader(), B);

							FFairyFontSdfPS::FParameters PP;
							PP.FontTexture = Batch.TextureRHI;
							PP.FontTextureSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
							PP.AlphaMaskThreshold = AlphaMaskThreshold;
							FRHIBatchedShaderParameters& BP = RHICmdList.GetScratchShaderParameters();
							SetShaderParameters(BP, FontSdfPS, PP);
							RHICmdList.SetBatchedShaderParameters(FontSdfPS.GetPixelShader(), BP);
						}
					}
					break;
					case EFairyRendererType::NoTexture:
					{
						FFairyNoTextureRenderVS::FParameters P;
						P.ViewProjectionMatrix = ViewProjection;
						P.ViewOffset = ViewOffset;
						P.ObjectDataBuffer = ObjSRV;
						FRHIBatchedShaderParameters& B = RHICmdList.GetScratchShaderParameters();
						SetShaderParameters(B, NoTexVS, P);
						RHICmdList.SetBatchedShaderParameters(NoTexVS.GetVertexShader(), B);

						FFairyNoTextureRenderPS::FParameters PP;
						PP.AlphaMaskThreshold = AlphaMaskThreshold;
						FRHIBatchedShaderParameters& BP = RHICmdList.GetScratchShaderParameters();
						SetShaderParameters(BP, NoTexPS, PP);
						RHICmdList.SetBatchedShaderParameters(NoTexPS.GetPixelShader(), BP);
					}
					break;
				}

				// === Draw ===
				RHICmdList.SetStreamSource(0, GlobalVB->GetRHI(), 0);
				RHICmdList.DrawIndexedPrimitive(GlobalIB->GetRHI(), Batch.BaseVertex, 0, Batch.NumVertices,
					Batch.FirstIndex, Batch.NumIndices / 3, 1);
			}
		});
}
