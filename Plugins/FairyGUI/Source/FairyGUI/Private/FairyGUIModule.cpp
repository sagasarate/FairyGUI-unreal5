// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FairyGUIModule.h"
#include "FairyApplication.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "UI/UIConfig.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FFairyGUIModule"

void FFairyGUIModule::StartupModule()
{
    // PostConfigInit 阶段：仅注册 Shader 路径（必须在 Shader 编译管线启动前）
    FString ShaderDir = FPaths::Combine(
        IPluginManager::Get().FindPlugin(TEXT("FairyGUI"))->GetBaseDir(),
        TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/FairyGUI"), ShaderDir);

#if WITH_EDITOR
    // Editor 尚未加载，推迟到引擎初始化完成后注册
    FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FFairyGUIModule::OnPostEngineInit);
#endif

    // 注册引擎关闭回调（所有构建类型）
    EnginePreExitHandle = FCoreDelegates::OnEnginePreExit.AddRaw(this, &FFairyGUIModule::OnEnginePreExit);
}

#if WITH_EDITOR
void FFairyGUIModule::OnPostEngineInit()
{
    EndPieDelegateHandle = FEditorDelegates::EndPIE.AddLambda([](bool boolSent) {
        UE_LOG(LogFairyGUI, Log, TEXT("Application destroy"));
        UFairyApplication::Destroy();
    });
}
#endif

void FFairyGUIModule::OnEnginePreExit()
{
    UE_LOG(LogFairyGUI, Log, TEXT("Engine pre-exit, destroying FairyGUI application"));
    UFairyApplication::Destroy();
}

void FFairyGUIModule::ShutdownModule()
{
    // 移除引擎关闭回调
    if (EnginePreExitHandle.IsValid())
        FCoreDelegates::OnEnginePreExit.Remove(EnginePreExitHandle);

#if WITH_EDITOR
    FEditorDelegates::EndPIE.Remove(EndPieDelegateHandle);
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFairyGUIModule, FairyGUI)
