#include "Widgets/Renderer/FairyNoTextureRenderShader.h"
#include "Widgets/Renderer/FairyFontShader.h"

IMPLEMENT_GLOBAL_SHADER(FFairyNoTextureRenderVS, "/Plugin/FairyGUI/Private/FairyNoTextureRenderShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FFairyNoTextureRenderPS, "/Plugin/FairyGUI/Private/FairyNoTextureRenderShader.usf", "MainPS", SF_Pixel);
