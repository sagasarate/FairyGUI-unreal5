// FairyFontShader.cpp
#include "Widgets/Renderer/FairyFontShader.h"
#include "Rendering/RenderingCommon.h"
#include "PipelineStateCache.h"

IMPLEMENT_GLOBAL_SHADER(FFairyFontVS, "/Plugin/FairyGUI/Private/FairyFontShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FFairyFontPS, "/Plugin/FairyGUI/Private/FairyFontShader.usf", "MainPS", SF_Pixel);

// Vertex Declaration：匹配 FSlateVertex 布局
void FFairyFontVertexDeclaration::InitRHI(FRHICommandListBase& RHICmdList)
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

void FFairyFontVertexDeclaration::ReleaseRHI()
{
    VertexDeclarationRHI.SafeRelease();
}

TGlobalResource<FFairyFontVertexDeclaration> GFairyFontVertexDeclaration;
