// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FairyGUIEditor.h"
#include "Editor.h"
#include "EditorReimportHandler.h"
#include "FairyGUIFactory.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "FFairyGUIEditorModule"

void FFairyGUIEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    auto Action = MakeShared<FAssetTypeActions_UIPackageAsset>();
    AssetTools.RegisterAssetTypeActions(Action);
    RegisteredAssetTypeActions.Add(Action);

	FReimportManager::Instance()->RegisterHandler(*GetMutableDefault<UFairyGUIFactory>());
}

void FFairyGUIEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	// 1. 检查模块是否仍然加载，防止引擎关闭顺序导致的崩溃
    if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
    {
        IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
        
        // 2. 遍历并反注册所有保存的 Action
        for (auto& Action : RegisteredAssetTypeActions)
        {
            if (Action.IsValid())
            {
                AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
            }
        }
    }
    RegisteredAssetTypeActions.Empty();

    if (!IsEngineExitRequested() && GIsEditor)
    {
        // 只有当 ReimportManager 模块还加载时才操作
        if (FModuleManager::Get().IsModuleLoaded(TEXT("UnrealEd")))
        {
            UFairyGUIFactory* Factory = GetMutableDefault<UFairyGUIFactory>();
            if (Factory)
            {
                FReimportManager::Instance()->UnregisterHandler(*Factory);
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFairyGUIEditorModule, FairyGUIEditor)