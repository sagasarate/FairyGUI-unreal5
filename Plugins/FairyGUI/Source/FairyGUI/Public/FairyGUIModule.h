// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FFairyGUIModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

#if WITH_EDITOR
    void OnPostEngineInit();
#endif

    void OnEnginePreExit();              // 新增方法声明
    FDelegateHandle EnginePreExitHandle; // 新增句柄

    FDelegateHandle EndPieDelegateHandle;
};
