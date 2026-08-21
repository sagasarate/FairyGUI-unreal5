#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class FFairyNoTextureRenderVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFairyNoTextureRenderVS);
    SHADER_USE_PARAMETER_STRUCT(FFairyNoTextureRenderVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER(FMatrix44f, ViewProjectionMatrix)
    SHADER_PARAMETER(FVector2f, ViewOffset)
    SHADER_PARAMETER_SRV(StructuredBuffer<FObjectData>, ObjectDataBuffer)
    END_SHADER_PARAMETER_STRUCT()
};

class FFairyNoTextureRenderPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FFairyNoTextureRenderPS);
    SHADER_USE_PARAMETER_STRUCT(FFairyNoTextureRenderPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER(float, AlphaMaskThreshold)
    END_SHADER_PARAMETER_STRUCT()
};
