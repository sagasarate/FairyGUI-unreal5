#pragma once

// #include "Slate.h"
#include "UI/FieldTypes.h"
#include "NTextFormat.generated.h"

UENUM(BlueprintType)
enum class SpecialStyle : uint8
{
	None,
	Superscript,
	Subscript
};

USTRUCT(BlueprintType)
struct FAIRYGUI_API FNTextFormat
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FName Face;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 Size;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool bBold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool bItalic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool bUnderline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	bool bStrikethrough;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 LineSpacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	int32 LetterSpacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	EAlignType Align;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	EVerticalAlignType VerticalAlign;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FColor OutlineColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	float OutlineSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FColor ShadowColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	FVector2D ShadowOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	SpecialStyle SpecialStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FairyGUI")
	TArray<FColor> GradientColor;

	FNTextFormat()
		: Size(0)
		, Color(FColor::White)
		, bBold(false)
		, bItalic(false)
		, bUnderline(false)
		, bStrikethrough(false)
		, LineSpacing(3)
		, LetterSpacing(0)
		, Align(EAlignType::Left)
		, VerticalAlign(EVerticalAlignType::Top)
		, OutlineColor(FColor::Black)
		, OutlineSize(0)
		, ShadowColor(FColor::Black)
		, ShadowOffset(0, 0)
		, SpecialStyle(SpecialStyle::None)
	{
	}
	bool EqualStyle(const FNTextFormat& AnotherFormat) const
	{
		return Size == AnotherFormat.Size && Color == AnotherFormat.Color && bBold == AnotherFormat.bBold
			&& bUnderline == AnotherFormat.bUnderline && bStrikethrough == AnotherFormat.bStrikethrough
			&& bItalic == AnotherFormat.bItalic && Align == AnotherFormat.Align
			&& SpecialStyle == AnotherFormat.SpecialStyle;
		;
	}
	// bool IsDifferentFontFormat(const FNTextFormat& AnotherFormat) const
	// {
	// 	return Size != AnotherFormat.Size || bBold != AnotherFormat.bBold || bItalic != AnotherFormat.bItalic
	// 		|| OutlineSize != AnotherFormat.OutlineSize || SpecialStyle != AnotherFormat.SpecialStyle;
	// }
};