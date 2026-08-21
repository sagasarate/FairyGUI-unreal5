#include "Widgets/Renderer/FairyRenderShader.h"
#include "Widgets/Renderer/FairyFontShader.h"
#include "Rendering/RenderingCommon.h"
#include "PipelineStateCache.h"

IMPLEMENT_GLOBAL_SHADER(FFairyRenderVS, "/Plugin/FairyGUI/Private/FairyRenderShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FFairyRenderPS, "/Plugin/FairyGUI/Private/FairyRenderShader.usf", "MainPS", SF_Pixel);

void FFairyRenderVertexDeclaration::InitRHI(FRHICommandListBase& RHICmdList)
{
    FVertexDeclarationElementList Elements;
    // ATTRIBUTE0: TexCoords[4] (float4)
    Elements.Add(FVertexElement(0, STRUCT_OFFSET(FSlateVertex, TexCoords), VET_Float4, 0, sizeof(FSlateVertex)));
    // ATTRIBUTE1: Position (FVector2f)
    Elements.Add(FVertexElement(0, STRUCT_OFFSET(FSlateVertex, Position), VET_Float2, 1, sizeof(FSlateVertex)));
    // ATTRIBUTE2: Color (FColor)
    Elements.Add(FVertexElement(0, STRUCT_OFFSET(FSlateVertex, Color), VET_Color, 2, sizeof(FSlateVertex)));
    VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
}

void FFairyRenderVertexDeclaration::ReleaseRHI()
{
    VertexDeclarationRHI.SafeRelease();
}

TGlobalResource<FFairyRenderVertexDeclaration> GFairyRenderVertexDeclaration;
