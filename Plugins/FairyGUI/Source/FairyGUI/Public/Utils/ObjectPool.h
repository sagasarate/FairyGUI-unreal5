#include "CoreMinimal.h"
#include "UI/FieldTypes.h"
#include "ObjectPool.generated.h"

class FPackageItem;

USTRUCT()
struct FObjectPoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UObject>> ObjectList;
};

USTRUCT()
struct FObjectPoolMapByPackageItem
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FString, FObjectPoolArray> ObjectMap;
};


UCLASS()
class FAIRYGUI_API UObjectPool : public UObject
{
	GENERATED_BODY()
protected:
	UPROPERTY()
	TMap<UClass*, FObjectPoolArray> ObjectMap;
	UPROPERTY()
	TMap<FString, FObjectPoolMapByPackageItem> ObjectMapByRes;
	UPROPERTY()
	TSet<TObjectPtr<UObject>> HoldObjects;

	int32 m_MaxPoolObject = 10240;

public:
	template <typename T> T* Borrow(bool bHold) { return Cast<T>(Borrow(T::StaticClass(), bHold)); }
	UObject*				 Borrow(UClass* Class, bool bHold);
	UObject*				 Borrow(EObjectType Type, bool bHold);
	UObject*				 Borrow(const FString& ResURL, bool bHold);
	void					 Return(UObject* Obj);
	void					 Return(TArray<UObject*>& Objs);
	void					 Unhold(UObject* Obj);
	void					 UnholdAll();
	void					 Clear(UClass* Class);
	void					 Clear(EObjectType Type);
	void					 Clear(const FString& ResURL);
	void					 ClearByPackage(const FString& PackageID);
	void					 ClearAll(bool bClearHold);
	void					 DisposeAllObjects();

	void SetMaxPoolObject(int32 Max) { m_MaxPoolObject = Max; }
};