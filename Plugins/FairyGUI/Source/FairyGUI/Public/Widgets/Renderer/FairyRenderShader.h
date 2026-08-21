#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FFairyRenderVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFairyRenderVS);
    SHADER_USE_PARAMETER_STRUCT(FFairyRenderVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER(FMatrix44f, ViewProjectionMatrix)
    SHADER_PARAMETER(FVector2f, ViewOffset)
    SHADER_PARAMETER_SRV(StructuredBuffer<FObjectData>, ObjectDataBuffer)
    END_SHADER_PARAMETER_STRUCT()
};

class FFairyRenderPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFairyRenderPS);
    SHADER_USE_PARAMETER_STRUCT(FFairyRenderPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_TEXTURE(Texture2D, RenderTexture)
    SHADER_PARAMETER_SAMPLER(SamplerState, RenderTextureSampler)
    SHADER_PARAMETER(float, AlphaMaskThreshold)
    END_SHADER_PARAMETER_STRUCT()
};

class FFairyRenderVertexDeclaration : public FRenderResource
{
public:
    FVertexDeclarationRHIRef VertexDeclarationRHI;
    virtual void             InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void             ReleaseRHI() override;
};

extern TGlobalResource<FFairyRenderVertexDeclaration> GFairyRenderVertexDeclaration;
