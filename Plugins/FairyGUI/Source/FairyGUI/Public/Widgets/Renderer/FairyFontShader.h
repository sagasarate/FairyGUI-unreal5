// FairyFontShader.h
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"



class FFairyFontVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFairyFontVS);
	SHADER_USE_PARAMETER_STRUCT(FFairyFontVS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER(FMatrix44f, ViewProjectionMatrix)
	SHADER_PARAMETER(FVector2f, ViewOffset)
	SHADER_PARAMETER_SRV(StructuredBuffer<FObjectData>, ObjectDataBuffer)
	END_SHADER_PARAMETER_STRUCT()
};

class FFairyFontPS : public FGlobalShader
{
public:
	DECLARE_EXPORTED_GLOBAL_SHADER(FFairyFontPS, FAIRYGUI_API);
	SHADER_USE_PARAMETER_STRUCT(FFairyFontPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, FontTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, FontTextureSampler)
	SHADER_PARAMETER(float, AlphaMaskThreshold)
	END_SHADER_PARAMETER_STRUCT()
};

class FFairyFontVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;
	virtual void			 InitRHI(FRHICommandListBase& RHICmdList) override;
	virtual void			 ReleaseRHI() override;
};

extern TGlobalResource<FFairyFontVertexDeclaration> GFairyFontVertexDeclaration;
