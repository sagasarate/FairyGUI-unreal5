#pragma once
#include "CoreMinimal.h"

inline FString GetLocalizationKey(const FString& ItemID, const FString& UserData)
{
	if (!UserData.IsEmpty())
	{
		int32 Start = INDEX_NONE;
		int32 End = INDEX_NONE;
		for (int32 Index = 0; Index < UserData.Len(); ++Index)
		{
			if (UserData[Index] == TEXT('`'))
			{
				if (Start == INDEX_NONE)
				{
					Start = Index;
				}
				else
				{
					End = Index;
					if (End > Start + 1)
						return UserData.Mid(Start + 1, End - Start - 1);
					else
						Start = INDEX_NONE;
				}
			}
		}
	}
	return ItemID;
}

inline FText ToLocText(const TCHAR* NameSpace, const TCHAR* Key, const TCHAR* Text)
{
	if (Text[0] == TEXT('`'))
		Text++;
	return FText::AsLocalizable_Advanced_LocText(NameSpace, Key, Text);
}