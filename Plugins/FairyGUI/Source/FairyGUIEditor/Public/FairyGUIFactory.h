#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "AssetTypeActions_Base.h"
#include "UIPackageAsset.h"
#include "FairyGUIFactory.generated.h"

UCLASS()
class FAIRYGUIEDITOR_API UFairyGUIFactory : public UFactory, public  FReimportHandler
{
	GENERATED_BODY()

public:
	UFairyGUIFactory();
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames)  override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>&  NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
};

class FAssetTypeActions_UIPackageAsset : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override { return INVTEXT("UI Package Asset"); }
    virtual UClass* GetSupportedClass() const override { return UUIPackageAsset::StaticClass(); }
    virtual FColor GetTypeColor() const override { return FColor::Magenta; }
    virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }

    // 必须返回 true，否则右键菜单不会出现“重新导入”选项
    virtual bool IsImportedAsset() const override { return true; }

    // 告诉引擎从哪里获取源文件路径
    virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeObjects, TArray<FString>& OutSourceFilePaths) const override
    {
        for (auto& Obj : TypeObjects)
        {
            if (auto* Asset = Cast<UUIPackageAsset>(Obj))
            {
                if (Asset->AssetImportData)
                {
                    Asset->AssetImportData->ExtractFilenames(OutSourceFilePaths);
                }
            }
        }
    }
};