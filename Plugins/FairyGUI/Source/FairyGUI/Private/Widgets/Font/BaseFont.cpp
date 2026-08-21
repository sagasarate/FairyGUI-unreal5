#include "Widgets/Font/BaseFont.h"
#include "FairyCommons.h"

FName G_DEFAULT_FONT_NAME(TEXT("FGUIDefaultFont"));

FName FBaseGlyphSequence::s_TypeName(TEXT("FBaseGlyphSequence"));
FName FBaseGlyphSequence::GetTypeNameStatic()
{
	return s_TypeName;
}

void FBaseGlyphSequence::Release()
{
	if (m_ParentFont.IsValid())
	{
		m_ParentFont->ReturnGlyph(SharedThis(this));
	}	
}

float UBaseFont::DEFAULT_ITALIC_ANGLE = 15.0f;