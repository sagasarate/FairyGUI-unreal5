#include "UI/Gears/GearText.h"
#include "UI/GObject.h"
#include "UI/GController.h"
#include "Utils/ByteBuffer.h"
#include "../../Utils/Localization.h"

FGearText::FGearText(UGObject* InOwner)
	: FGearBase(InOwner)
{
	Type = EType::Text;
}

FGearText::~FGearText()
{
}

void FGearText::Init()
{
	Default = Owner->GetText();
	Storage.Reset();
}

void FGearText::AddStatus(const FString& PageID, FByteBuffer* Buffer)
{
	auto&	str = Buffer->ReadS();
	FString Key = GetLocalizationKey(Owner->ID, Owner->UserData.AsString());

	if (PageID.IsEmpty())
	{
		auto txt = ToLocText(*Owner->GetPackageName(), *FString::Printf(TEXT("%s_gear_%s"), *Key, *PageID), *str);
		Default = txt;
	}
	else
	{
		auto txt = ToLocText(*Owner->GetPackageName(), *FString::Printf(TEXT("%s_gear_dft"), *Key), *str);
		Storage.Add(PageID, txt);
	}
}

void FGearText::Apply()
{
	FText* Value = Storage.Find(Controller->GetSelectedPageID());
	if (Value == nullptr)
		Value = &Default;

	Owner->bGearLocked = true;
	Owner->SetText(*Value);
	Owner->bGearLocked = false;
}

void FGearText::UpdateState()
{
	Storage.Add(Controller->GetSelectedPageID(), Owner->GetText());
}
