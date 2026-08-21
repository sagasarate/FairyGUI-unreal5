// FairyFontSdfShader.h — SDF/MSDF 文字渲染 Shader
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "Widgets/Renderer/FairyFontShader.h" // FObjectData, FFairyFontVertexDeclaration

class FFairyFontSdfVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFairyFontSdfVS);
	SHADER_USE_PARAMETER_STRUCT(FFairyFontSdfVS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER(FMatrix44f, ViewProjectionMatrix)
	SHADER_PARAMETER(FVector2f, ViewOffset)
	SHADER_PARAMETER_SRV(StructuredBuffer<FObjectData>, ObjectDataBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FFairyFontSdfPS : public FGlobalShader
{
public:
	DECLARE_EXPORTED_GLOBAL_SHADER(FFairyFontSdfPS, FAIRYGUI_API);
	SHADER_USE_PARAMETER_STRUCT(FFairyFontSdfPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, FontTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, FontTextureSampler)
	SHADER_PARAMETER(float, AlphaMaskThreshold)
	END_SHADER_PARAMETER_STRUCT()
};
