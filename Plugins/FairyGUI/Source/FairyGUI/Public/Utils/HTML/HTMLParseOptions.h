#pragma once

#include "CoreMinimal.h"

struct FHTMLParseOptions
{
	bool   bLinkUnderline;
	FColor LinkColor;
	FColor LinkBgColor;
	FColor LinkHoverBgColor;
	bool   bIgnoreWhiteSpace;

	static bool	  DefaultLinkUnderline;
	static FColor DefaultLinkColor;
	static FColor DefaultLinkBgColor;
	static FColor DefaultLinkHoverBgColor;

	FHTMLParseOptions()
	{
		bLinkUnderline = DefaultLinkUnderline;
		LinkColor = DefaultLinkColor;
		LinkBgColor = DefaultLinkBgColor;
		LinkHoverBgColor = DefaultLinkHoverBgColor;
		bIgnoreWhiteSpace = false;
	}
};
