#include "Widgets/Font/FontManager.h"
#include "Widgets/Font/DynamicFont.h"
#include "Widgets/Font/BitmapFont.h"
#include "UI/UIPackage.h"
#include "UI/PackageItem.h"

UBaseFont* UFontManager::GetFont(FName Name)
{
	if (m_FontMap.Contains(Name))
	{
		return m_FontMap[Name];
	}
	else if (Name.ToString().StartsWith(TEXT("ui://")))
	{
		TSharedPtr<FPackageItem> FontItem = UUIPackage::GetItemByURL(Name.ToString());
		if (FontItem.IsValid())
		{
			FontItem->Load();
			if (FontItem->BitmapFont)
			{
				RegisterFont(FontItem->BitmapFont, Name);
				return FontItem->BitmapFont;
			}
		}
	}

	auto pFont = m_FontMap.Find(G_DEFAULT_FONT_NAME);
	if (pFont == nullptr)
	{
		auto Font = NewObject<UDynamicFont>(this);
		RegisterFont(Font);
		return Font;
	}
	return *pFont;
}