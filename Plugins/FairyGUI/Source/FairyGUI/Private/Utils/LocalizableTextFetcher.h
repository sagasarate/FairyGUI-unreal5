#include "CoreMinimal.h"
#include "UI/PackageItem.h"
#include "Utils/ByteBuffer.h"

class FLocalizableTextFetcher
{
protected:
	struct FComponentInfo
	{
		EObjectType				ObjectType;
		TSharedPtr<FByteBuffer> Buffer;
		FComponentInfo(EObjectType InObjectType, TSharedPtr<FByteBuffer> InBuffer)
			: ObjectType(InObjectType), Buffer(InBuffer) {}
		FComponentInfo()
			: ObjectType(EObjectType::Component), Buffer(nullptr) {}
	};

public:
	static void FetchLocalizableTexts(const TArray<uint8>& PackageData, TMap<FString, FString>& OutTexts);

protected:
	static void FetchLocalizableTextsFromComponent(const FString& ID, EObjectType ObjectType, FByteBuffer& Buffer, TMap<FString, FComponentInfo>& ComponentBuffers, TMap<FString, FString>& OutTexts);
	static FString FetchLocalizableTextsFromItem(EObjectType ObjectType, const FString& SrcID, FByteBuffer& Buffer, int32 StartPos, TMap<FString, FComponentInfo>& ComponentBuffers, TMap<FString, FString>& OutTexts);
	static void FetchLocalizableTextsFromTransition(const FString& OwnerID, const TArray<FString>& TargetIDs, FByteBuffer& Buffer, TMap<FString, FString>& OutTexts);
	static void FetchLocalizableTextsFromTransitionValue(const FString& OwnerID, const FString& TransName, const FString& TargetID, ETransitionActionType ItemType, FByteBuffer& Buffer, TMap<FString, FString>& OutTexts);
	static void FetchLocalizableText(const FString& Key, const FString& Text, TMap<FString, FString>& OutTexts);
};