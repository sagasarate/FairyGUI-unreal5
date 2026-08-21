#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "FieldTypes.h"
#include "UIConfig.generated.h"

UENUM(BlueprintType)
enum class EBitmapFontOutlineType : uint8
{
	FourDir,
	EightDir,
};

USTRUCT(BlueprintType)
struct FAIRYGUI_API FUIConfig
{
	GENERATED_USTRUCT_BODY()

public:
	static FUIConfig Config;

	FUIConfig();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	TSoftObjectPtr<UFont> DefaultFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString ButtonSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	float ButtonSoundVolumeScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 DefaultScrollStep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	float DefaultScrollDecelerationRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool DefaultScrollTouchEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool DefaultScrollBounceEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	EScrollBarDisplayType DefaultScrollBarDisplay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString VerticalScrollBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString HorizontalScrollBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 TouchDragSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 ClickDragSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 TouchScrollSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 DefaultComboBoxVisibleItemCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString GlobalModalWaiting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FColor ModalLayerColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString TooltipsWin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool BringWindowToFrontOnClick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString WindowModalWaiting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString PopupMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FString PopupMenuSeperator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 HTMLElementPoolGrowLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 FontGlyphPoolGrowLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 TextLineInfoPoolGrowLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 RenderUnitPoolGrowLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool OutlineSizeIsSDFOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	float InputCaretSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FColor InputHighlightColor;

	// 位图字体描边方向数：4 = 上下左右，8 = 八方向（含对角线）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	EBitmapFontOutlineType BitmapFontOutlineType;
};