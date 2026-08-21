// FairyFontSdfShader.cpp
#include "Widgets/Renderer/FairyFontSdfShader.h"
#include "Rendering/RenderingCommon.h"

IMPLEMENT_GLOBAL_SHADER(FFairyFontSdfVS, "/Plugin/FairyGUI/Private/FairyFontSdfShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FFairyFontSdfPS, "/Plugin/FairyGUI/Private/FairyFontSdfShader.usf", "MainPS", SF_Pixel);
