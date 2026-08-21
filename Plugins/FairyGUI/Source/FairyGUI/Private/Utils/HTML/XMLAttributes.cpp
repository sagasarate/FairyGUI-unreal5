#include "Utils/HTML/XMLAttributes.h"

extern const FString FAIRYGUI_API G_EMPTY_STRING;

const FString& FXMLAttributes::Get(const FString& AttrName, const FString& DefaultValue)
{
	FString* Result = Find(AttrName);
	if (Result != nullptr)
		return *Result;
	else
		return DefaultValue;
}

int32 FXMLAttributes::GetInt(const FString& AttrName, int32 DefaultValue)
{
	const FString& Value = Get(AttrName);
	if (Value.Len() == 0)
		return DefaultValue;

	return FCString::Atoi(*Value);
}

float FXMLAttributes::GetFloat(const FString& AttrName, float DefaultValue)
{
	const FString& Value = Get(AttrName);
	if (Value.Len() == 0)
		return DefaultValue;

	return FCString::Atof(*Value);
}

bool FXMLAttributes::GetBool(const FString& AttrName, bool DefaultValue)
{
	const FString& Value = Get(AttrName);
	if (Value.Len() == 0)
		return DefaultValue;

	return Value.ToBool();
}

FColor FXMLAttributes::GetColor(const FString& AttrName, const FColor& DefaultValue)
{
	const FString& Value = Get(AttrName);
	if (Value.Len() == 0)
		return DefaultValue;

	return FColor::FromHex(Value);
}

void FXMLAttributes::Set(const FString& AttrName, const FString& Value)
{
	Add(AttrName, Value);
}
void FXMLAttributes::Set(const FString& AttrName, int32 Value)
{
	Add(AttrName, LexToString(Value));
}
void FXMLAttributes::Set(const FString& AttrName, float Value)
{
	Add(AttrName, LexToString(Value));
}
void FXMLAttributes::Set(const FString& AttrName, bool Value)
{
	Add(AttrName, LexToString(Value));
}
void FXMLAttributes::Set(const FString& AttrName, const FColor& Value)
{
	Add(AttrName, Value.ToHex());
}