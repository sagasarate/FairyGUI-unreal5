// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeActions_Base.h"

class FFairyGUIEditorModule : public IModuleInterface
{
protected:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
