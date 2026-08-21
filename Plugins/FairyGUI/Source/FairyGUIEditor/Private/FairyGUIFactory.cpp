#include "FairyGUIFactory.h"
#include "Serialization/BufferArchive.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"


UFairyGUIFactory::UFairyGUIFactory()
{
    SupportedClass = UUIPackageAsset::StaticClass();
    bEditorImport = true;
    bAutomatedReimport = true;
    bCreateNew = false;
    bText = false;
    Formats.Add(TEXT("fui;FairyGUI package files"));
}

UObject* UFairyGUIFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
    UUIPackageAsset* UIAsset = NewObject<UUIPackageAsset>(InParent, InName, Flags);

    const int32 InDataSize = BufferEnd - Buffer;
    UIAsset->Data.Empty(InDataSize);
    UIAsset->Data.AddUninitialized(InDataSize);
    FMemory::Memcpy(UIAsset->Data.GetData(), Buffer, InDataSize);

    if (!UIAsset->AssetImportData)
    {
        UIAsset->AssetImportData = NewObject<UAssetImportData>(UIAsset, UAssetImportData::StaticClass());
    }
    UIAsset->AssetImportData->Update(CurrentFilename);

    return UIAsset;
}

bool UFairyGUIFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UUIPackageAsset* UIAsset = Cast<UUIPackageAsset>(Obj);
    if (UIAsset && UIAsset->AssetImportData)
    {
        UIAsset->AssetImportData->ExtractFilenames(OutFilenames);
        return true;
    }
    return false;
}

void UFairyGUIFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UUIPackageAsset* UIAsset = Cast<UUIPackageAsset>(Obj);
    if (UIAsset && NewReimportPaths.Num() > 0 && UIAsset->AssetImportData)
    {
        UIAsset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
    }
}

EReimportResult::Type UFairyGUIFactory::Reimport(UObject* Obj)
{
    UUIPackageAsset* UIAsset = Cast<UUIPackageAsset>(Obj);
    
    // 1. 基础合法性检查
    if (!UIAsset || !UIAsset->AssetImportData)
    {
        return EReimportResult::Failed;
    }

    // 2. 获取存储的原始路径
    const FString Filename = UIAsset->AssetImportData->GetFirstFilename();

    // 3. 检查文件是否还在磁盘上
    if (Filename.IsEmpty() || !FPaths::FileExists(Filename))
    {
        return EReimportResult::Failed;
    }

    // 4. 直接读取二进制数据到资产的 Data 成员中
    // 这样避免了调用 StaticImportObject 导致的资产覆盖或路径丢失问题
    if (FFileHelper::LoadFileToArray(UIAsset->Data, *Filename)) // 假设你的成员变量名是 PackageData
    {
        // 5. 【极其重要】成功后必须更新导入元数据
        // 这步不执行，UE 就会认为路径失效，从而导致右键菜单消失
        UIAsset->AssetImportData->Update(Filename);

        // 6. 标记资产已更新
        UIAsset->MarkPackageDirty();
        
        return EReimportResult::Succeeded;
    }

    return EReimportResult::Failed;
}
int32 UFairyGUIFactory::GetPriority() const
{
    return 100; 
}