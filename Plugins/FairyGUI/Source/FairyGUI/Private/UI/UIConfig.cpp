#include "UI/UIConfig.h"
#include "FairyApplication.h"
#include "Widgets/Font/BaseFont.h"

FUIConfig FUIConfig::Config;

FUIConfig::FUIConfig()
	: DefaultFont(FSoftObjectPath(TEXT("/Engine/EngineFonts/Roboto.Roboto")))
	, ButtonSoundVolumeScale(1)
	, DefaultScrollStep(25)
	, DefaultScrollDecelerationRate(0.967f)
	, DefaultScrollTouchEffect(true)
	, DefaultScrollBounceEffect(true)
	, DefaultScrollBarDisplay(EScrollBarDisplayType::Default)
	, TouchDragSensitivity(10)
	, ClickDragSensitivity(2)
	, TouchScrollSensitivity(20)
	, DefaultComboBoxVisibleItemCount(10)
	, ModalLayerColor(0, 0, 0, 120)
	, BringWindowToFrontOnClick(true)
	, HTMLElementPoolGrowLimit(256)
	, FontGlyphPoolGrowLimit(256)
	, TextLineInfoPoolGrowLimit(256)
	, RenderUnitPoolGrowLimit(256)
	, OutlineSizeIsSDFOffset(false)
	, InputCaretSize(1.0f)
	, InputHighlightColor(255, 223, 141, 128)
	, BitmapFontOutlineType(EBitmapFontOutlineType::EightDir)
{
}