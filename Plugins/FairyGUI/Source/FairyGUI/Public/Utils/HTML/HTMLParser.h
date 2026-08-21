#pragma once

#include "Utils/HTML/HTMLElement.h"
#include "Utils/HTML/HTMLParseOptions.h"



class FAIRYGUI_API FHTMLParser
{
public:
	static FHTMLParser		 DefaultParser;
	static FHTMLParseOptions DefaultParseOptions;

	FHTMLParser();

	void Parse(const FString& InText, const FNTextFormat& InFormat, TArray<FHTMLElement*>& OutElements,
		const FHTMLParseOptions& InParseOptions);

protected:
	void PushTextFormat();
	void PopTextFormat();
	bool IsNewLine();
	void AppendText(const FString& InText, int32 CharIndex);

	struct FMyTextFormat : FNTextFormat
	{
		bool bColorChanged;
	};
	TArray<FMyTextFormat>  TextFormatStack;
	FMyTextFormat		   Format;
	TArray<FHTMLElement*>* Elements;
	FHTMLParseOptions	   ParseOptions;
};