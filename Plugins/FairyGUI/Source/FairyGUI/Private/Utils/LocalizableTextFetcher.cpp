#include "LocalizableTextFetcher.h"
#include "UI/Gears/GearBase.h"
#include "Localization.h"

void FLocalizableTextFetcher::FetchLocalizableTexts(const TArray<uint8>& PackageData, TMap<FString, FString>& OutTexts)
{
	FByteBuffer Buffer(PackageData.GetData(), 0, PackageData.Num(), false);
	if (Buffer.ReadUint() != 0x46475549)
	{
		UE_LOG(LogFairyGUI, Error, TEXT("not valid package format in %d"), Buffer.ReadUint());
		return;
	}

	Buffer.Version = Buffer.ReadInt();
	Buffer.ReadBool(); // compressed
	Buffer.SkipString();
	Buffer.SkipString();
	Buffer.Skip(20);
	int32 indexTablePos = Buffer.GetPos();
	int32 cnt;

	Buffer.Seek(indexTablePos, 4);

	cnt = Buffer.ReadInt();
	TArray<FString>* StringTable = new TArray<FString>();
	StringTable->SetNum(cnt, EAllowShrinking::Yes);
	for (int32 i = 0; i < cnt; i++)
	{
		(*StringTable)[i] = Buffer.ReadString();
	}
	Buffer.StringTable = MakeShareable(StringTable);

	Buffer.Seek(indexTablePos, 1);

	TMap<FString, FComponentInfo> ComponentBuffers;

	cnt = Buffer.ReadShort();
	for (int32 i = 0; i < cnt; i++)
	{
		int32 nextPos = Buffer.ReadInt();
		nextPos += Buffer.GetPos();

		EPackageItemType Type = (EPackageItemType)Buffer.ReadByte();
		auto			 ID = Buffer.ReadS();
		auto			 Name = Buffer.ReadS();
		Buffer.Skip(13);
		switch (Type)
		{
			case EPackageItemType::Component:
			{
				int32		extension = Buffer.ReadByte();
				EObjectType ObjectType = EObjectType::Component;
				if (extension > 0)
					ObjectType = (EObjectType)extension;
				auto RawData = Buffer.ReadBuffer(false);
				ComponentBuffers.Add(ID, FComponentInfo(ObjectType, RawData));
				break;
			}
			default:
				break;
		}

		Buffer.SetPos(nextPos);
	}

	for (auto It = ComponentBuffers.CreateIterator(); It; ++It)
	{
		FetchLocalizableTextsFromComponent(
			It->Key, It->Value.ObjectType, *It->Value.Buffer, ComponentBuffers, OutTexts);
	}
}

void FLocalizableTextFetcher::FetchLocalizableTextsFromComponent(const FString& ID, EObjectType ObjectType,
	FByteBuffer& Buffer, TMap<FString, FComponentInfo>& ComponentBuffers, TMap<FString, FString>& OutTexts)
{
	Buffer.Seek(0, 2);

	int32			childCount = Buffer.ReadShort();
	TArray<FString> ChildIDs;
	ChildIDs.SetNum(childCount);
	for (int32 i = 0; i < childCount; i++)
	{
		int32 dataLen = Buffer.ReadShort();
		int32 curPos = Buffer.GetPos();

		Buffer.Seek(curPos, 0);

		EObjectType	   type = (EObjectType)Buffer.ReadByte();
		const FString& src = Buffer.ReadS();
		Buffer.Skip(2);

		ChildIDs[i] = FetchLocalizableTextsFromItem(type, src, Buffer, curPos, ComponentBuffers, OutTexts);

		Buffer.SetPos(curPos + dataLen);
	}

	Buffer.Seek(0, 5);

	int32 transitionCount = Buffer.ReadShort();
	for (int32 i = 0; i < transitionCount; i++)
	{
		int32 nextPos = Buffer.ReadShort();
		nextPos += Buffer.GetPos();
		FetchLocalizableTextsFromTransition(ID, ChildIDs, Buffer, OutTexts);
		Buffer.SetPos(nextPos);
	}
}

FString FLocalizableTextFetcher::FetchLocalizableTextsFromItem(EObjectType ObjectType, const FString& SrcID,
	FByteBuffer& Buffer, int32 StartPos, TMap<FString, FComponentInfo>& ComponentBuffers,
	TMap<FString, FString>& OutTexts)
{
	Buffer.Seek(StartPos, 0);
	Buffer.Skip(5);
	auto ID = Buffer.ReadS();
	auto Name = Buffer.ReadS();

	Buffer.Skip(8);
	if (Buffer.ReadBool())
		Buffer.Skip(8);
	if (Buffer.ReadBool())
		Buffer.Skip(16);
	if (Buffer.ReadBool())
		Buffer.Skip(8);
	if (Buffer.ReadBool())
		Buffer.Skip(8);
	if (Buffer.ReadBool())
		Buffer.Skip(9);
	Buffer.Skip(13);

	const FString& UserData = Buffer.ReadS();
	FString		   Key = GetLocalizationKey(ID, UserData);

	{
		Buffer.Seek(StartPos, 1);
		const FString& str = Buffer.ReadS();
		if (!str.IsEmpty())
			FetchLocalizableText(FString::Printf(TEXT("%s_tooltips"), *Key), str, OutTexts);
	}

	{
		Buffer.Seek(StartPos, 2);
		int16 cnt = Buffer.ReadShort();
		for (int32 i = 0; i < cnt; i++)
		{
			int16 nextPos = Buffer.ReadShort();
			nextPos += Buffer.GetPos();
			int8 GearType = Buffer.ReadByte();
			if (GearType == (int8)FGearBase::EType::Text)
			{
				Buffer.Skip(2);
				int32 Count = Buffer.ReadShort();
				for (int32 j = 0; j < Count; j++)
				{
					const FString& page = Buffer.ReadS();
					if (page.IsEmpty())
						continue;
					const FString& str = Buffer.ReadS();
					FetchLocalizableText(FString::Printf(TEXT("%s_gear_%s"), *Key, *page), str, OutTexts);
				}
				if (Buffer.ReadBool())
				{
					const FString& str = Buffer.ReadS();
					FetchLocalizableText(FString::Printf(TEXT("%s_gear_dft"), *Key), str, OutTexts);
				}
			}

			Buffer.SetPos(nextPos);
		}
	}

	switch (ObjectType)
	{
		case EObjectType::InputText:
			if (true)
			{
				Buffer.Seek(StartPos, 4);
				const FString* str;
				if ((str = Buffer.ReadSP()) != nullptr)
					FetchLocalizableText(FString::Printf(TEXT("%s_prompt"), *Key), *str, OutTexts);
			}
		case EObjectType::Text:
		case EObjectType::RichText:
			if (true)
			{
				Buffer.Seek(StartPos, 6);
				const FString& str = Buffer.ReadS();
				if (!str.IsEmpty())
					FetchLocalizableText(FString::Printf(TEXT("%s_text"), *Key), str, OutTexts);
			}
			break;
		case EObjectType::Component:
			if (true)
			{
				Buffer.Seek(StartPos, 4);
				Buffer.Skip(2);
				int32 cnt = Buffer.ReadShort();
				for (int32 i = 0; i < cnt; i++)
					Buffer.Skip(4);

				if (Buffer.Version >= 2)
				{
					cnt = Buffer.ReadShort();
					for (int32 i = 0; i < cnt; i++)
					{
						FString		  Target = Buffer.ReadS();
						EObjectPropID PropID = (EObjectPropID)Buffer.ReadShort();
						FString		  Value = Buffer.ReadS();
						if (PropID == EObjectPropID::Text)
							FetchLocalizableText(FString::Printf(TEXT("%s_comp_%s"), *Key, *Target), Value, OutTexts);
					}
				}
			}
			break;
		case EObjectType::List:
		case EObjectType::Tree:
			if (true)
			{
				Buffer.Seek(StartPos, 8);
				Buffer.Skip(2);
				int32		   itemCount = Buffer.ReadShort();
				const FString* pStr;
				for (int32 i = 0; i < itemCount; i++)
				{
					int32 nextPos = Buffer.ReadShort();
					nextPos += Buffer.GetPos();
					Buffer.Skip(2);

					if ((pStr = Buffer.ReadSP()) != nullptr && !pStr->IsEmpty())
						FetchLocalizableText(FString::Printf(TEXT("%s_text_li%d"), *Key, i), *pStr, OutTexts);
					if ((pStr = Buffer.ReadSP()) != nullptr && !pStr->IsEmpty())
						FetchLocalizableText(FString::Printf(TEXT("%s_stext_li%d"), *Key, i), *pStr, OutTexts);

					Buffer.Skip(6);

					int32 cnt = Buffer.ReadShort();
					Buffer.Skip(cnt * 4);

					if (Buffer.Version >= 2)
					{
						cnt = Buffer.ReadShort();
						for (int32 j = 0; j < cnt; j++)
						{
							FString		  Target = Buffer.ReadS();
							EObjectPropID PropID = (EObjectPropID)Buffer.ReadShort();
							FString		  Value = Buffer.ReadS();
							if (PropID == EObjectPropID::Text)
								FetchLocalizableText(
									FString::Printf(TEXT("%s_li%d_%s"), *Key, i, *Target), Value, OutTexts);
						}
					}
					Buffer.SetPos(nextPos);
				}
			}
			break;
		case EObjectType::Label:
			if (Buffer.Seek(StartPos, 6))
			{
				if ((EObjectType)Buffer.ReadByte() != ObjectType)
					break;
				const FString* pStr;
				if ((pStr = Buffer.ReadSP()) != nullptr && !pStr->IsEmpty())
					FetchLocalizableText(FString::Printf(TEXT("%s_text"), *Key), *pStr, OutTexts);
				Buffer.Skip(2);
				if (Buffer.ReadBool())
					Buffer.Skip(4);
				Buffer.Skip(4);
				if (Buffer.ReadBool())
				{
					if ((pStr = Buffer.ReadSP()) != nullptr)
						FetchLocalizableText(FString::Printf(TEXT("%s_prompt"), *Key), *pStr, OutTexts);
				}
			}
			break;
		case EObjectType::Button:
			if (Buffer.Seek(StartPos, 6))
			{
				if ((EObjectType)Buffer.ReadByte() != ObjectType)
					break;
				const FString* pStr;
				if ((pStr = Buffer.ReadSP()) != nullptr && !pStr->IsEmpty())
					FetchLocalizableText(FString::Printf(TEXT("%s_text"), *Key), *pStr, OutTexts);
				if ((pStr = Buffer.ReadSP()) != nullptr && !pStr->IsEmpty())
					FetchLocalizableText(FString::Printf(TEXT("%s_stext"), *Key), *pStr, OutTexts);
			}
			break;
		case EObjectType::ComboBox:
			if (Buffer.Seek(StartPos, 6))
			{
				if ((EObjectType)Buffer.ReadByte() != ObjectType)
					break;
				const FString* pStr;

				int32 itemCount = Buffer.ReadShort();
				for (int32 i = 0; i < itemCount; i++)
				{
					int32 nextPos = Buffer.ReadShort();
					nextPos += Buffer.GetPos();

					pStr = Buffer.ReadSP();
					if (pStr != nullptr && !pStr->IsEmpty())
						FetchLocalizableText(FString::Printf(TEXT("%s_cbi%d_text"), *Key, i), *pStr, OutTexts);
					Buffer.SetPos(nextPos);
				}

				if ((pStr = Buffer.ReadSP()) != nullptr && !pStr->IsEmpty())
					FetchLocalizableText(FString::Printf(TEXT("%s_text"), *Key), *pStr, OutTexts);
			}
			break;
	}
	return ID;
}

void FLocalizableTextFetcher::FetchLocalizableTextsFromTransition(
	const FString& OwnerID, const TArray<FString>& TargetIDs, FByteBuffer& Buffer, TMap<FString, FString>& OutTexts)
{
	auto TransName = Buffer.ReadS();
	Buffer.Skip(13);

	int32 cnt = Buffer.ReadShort();
	for (int32 i = 0; i < cnt; i++)
	{
		int32 dataLen = Buffer.ReadShort();
		int32 curPos = Buffer.GetPos();

		Buffer.Seek(curPos, 0);
		auto ItemType = (ETransitionActionType)Buffer.ReadByte();
		Buffer.Skip(4);
		int32 TargetIndex = Buffer.ReadShort();
		if (TargetIndex >= 0 && TargetIndex < TargetIDs.Num())
		{
			auto& TargetID = TargetIDs[TargetIndex];
			Buffer.Skip(2);
			bool hasTween = Buffer.ReadBool();
			Buffer.Seek(curPos, 2);
			FetchLocalizableTextsFromTransitionValue(OwnerID, TransName, TargetID, ItemType, Buffer, OutTexts);

			if (hasTween)
			{
				Buffer.Seek(curPos, 3);
				FetchLocalizableTextsFromTransitionValue(OwnerID, TransName, TargetID, ItemType, Buffer, OutTexts);
			}
		}
		Buffer.SetPos(curPos + dataLen);
	}
}

void FLocalizableTextFetcher::FetchLocalizableTextsFromTransitionValue(const FString& OwnerID, const FString& TransName,
	const FString& TargetID, ETransitionActionType ItemType, FByteBuffer& Buffer, TMap<FString, FString>& OutTexts)
{
	if (ItemType == ETransitionActionType::Text)
	{
		auto Str = Buffer.ReadS();
		if (!Str.IsEmpty())
			OutTexts.Add(FString::Printf(TEXT("%s_%s_trans_text"), *OwnerID, *TransName, *TargetID), Str);
	}
}

void FLocalizableTextFetcher::FetchLocalizableText(
	const FString& Key, const FString& Text, TMap<FString, FString>& OutTexts)
{
	if (Text.Len() > 1 && Text[0] == TEXT('`') && Text[1] != TEXT('`'))
		OutTexts.Add(Key, Text.Right(Text.Len() - 1));
}
