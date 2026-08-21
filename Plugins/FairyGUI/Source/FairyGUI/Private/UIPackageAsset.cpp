#include "UIPackageAsset.h"
#include "EditorFramework/AssetImportData.h"
#include "Utils/LocalizableTextFetcher.h"

UUIPackageAsset::UUIPackageAsset()
{
#if WITH_EDITORONLY_DATA
	AssetImportData = CreateDefaultSubobject<UAssetImportData>(TEXT("AssetImportData"));
#endif
}

#if WITH_EDITORONLY_DATA

void UUIPackageAsset::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject) && !AssetImportData)
	{
		AssetImportData = NewObject<UAssetImportData>(this, UAssetImportData::StaticClass());
	}
}

void UUIPackageAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	if (AssetImportData)
	{
		Context.AddTag(FAssetRegistryTag(
			SourceFileTagName(),
			AssetImportData->GetSourceData().ToJson(),
			FAssetRegistryTag::TT_Hidden));
	#if WITH_EDITOR
		AssetImportData->AppendAssetRegistryTags(Context);
	#endif
	}
	Super::GetAssetRegistryTags(Context);
}
#endif

#if WITH_EDITOR
void UUIPackageAsset::RefreshLocalizableTextCache()
{
	LocalizableTextCache.Empty();

	FString Namespace = GetName();

	TMap<FString, FString> TextsToLocalize;

	FLocalizableTextFetcher::FetchLocalizableTexts(Data, TextsToLocalize);

	for (auto It = TextsToLocalize.CreateIterator(); It; ++It)
	{
		FText LocalizedText = FText::AsLocalizable_Advanced_LocText(*Namespace, *It->Key, *It->Value);
		LocalizableTextCache.Add(LocalizedText);
	}
}

void UUIPackageAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshLocalizableTextCache();
}
#endif