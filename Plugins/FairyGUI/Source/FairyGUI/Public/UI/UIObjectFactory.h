#pragma once

#include "CoreMinimal.h"
#include "FieldTypes.h"
#include "PackageItem.h"
#include "UIObjectFactory.generated.h"

class UGComponent;
class UGLoader;
class UGObject;
class FPackageItem;

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(class UGComponent*, FDynGComponentCreator, UObject*, outer);

UCLASS(BlueprintType)
class FAIRYGUI_API UUIObjectFactory : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	static void SetExtension(const FString& URL, FDynGComponentCreator Creator);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	static void SetExtensionWithClass(const FString& URL, TSubclassOf<UGComponent> ClassType);

	static UGObject* NewObject(const TSharedPtr<FPackageItem>& PackageItem, UObject* Outer, TSubclassOf<UGObject> ObjClass);
	static UGObject* NewObject(EObjectType Type, UObject* Outer, TSubclassOf<UGObject> ObjClass);
	static UClass*	 GetClass(EObjectType Type);

	static void ResolvePackageItemExtension(const TSharedPtr<FPackageItem>& PackageItem);

public:
	static TMap<FString, FGComponentCreator> PackageItemExtensions;
	static TSubclassOf<class UGLoader>		 LoaderExtension;

	friend class UFGUIPackage;
};
