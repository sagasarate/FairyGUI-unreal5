#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EditorFramework/AssetImportData.h"
#include "UIPackageAsset.generated.h"

UCLASS()
class FAIRYGUI_API UUIPackageAsset : public UObject
{
	GENERATED_BODY()

public:
	UUIPackageAsset();
	UPROPERTY(EditAnywhere)
	TArray<uint8> Data;

	UPROPERTY(EditAnywhere, Category = "Localization", meta = (ContentDir))
	TArray<FText> LocalizableTextCache;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void RefreshLocalizableTextCache();
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = "ImportSettings")
	TObjectPtr<UAssetImportData> AssetImportData;

	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

	/** Return the AssetImportData for editor reimport support */
	UAssetImportData* GetAssetImportData() const { return AssetImportData; }
#endif
};