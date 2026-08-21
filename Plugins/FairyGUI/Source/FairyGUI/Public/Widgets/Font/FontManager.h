#pragma once

#include "CoreMinimal.h"
#include "Widgets/Font/BaseFont.h"
#include "FairyCommons.h"
#include "FontManager.generated.h"

UCLASS()
class FAIRYGUI_API UFontManager : public UObject
{
	GENERATED_BODY()
protected:
	UPROPERTY()
	TMap<FName, TObjectPtr<UBaseFont>> m_FontMap;

public:
	void RegisterFont(UBaseFont* Font, FName Alias = NAME_None)
	{
		m_FontMap.Add(Font->GetName(), Font);
		if (!Alias.IsNone())
			m_FontMap.Add(Alias, Font);
	}
	void UnregisterFont(UBaseFont* Font)
	{
		for (auto It = m_FontMap.CreateIterator(); It; ++It)
		{
			if (It->Value == Font)
			{
				It.RemoveCurrent();
			}
		}
	}

	UBaseFont* GetFont(FName Name);

	void Clear() { m_FontMap.Empty(); }

protected:
};
