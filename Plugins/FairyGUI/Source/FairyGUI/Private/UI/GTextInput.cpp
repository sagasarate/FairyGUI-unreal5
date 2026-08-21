#include "UI/GTextInput.h"
#include "Utils/ByteBuffer.h"
#include "../Utils/Localization.h"

UGTextInput::UGTextInput() {}

void UGTextInput::CreateDisplayObject()
{
	DisplayObject = Content = InputContent = MakeShared<SInputTextField>(this);
}

UGTextInput::~UGTextInput() {}

void UGTextInput::Dispose()
{
	if (bDisposed)
		return;
	InputContent.Reset();
	UGRichTextField::Dispose();
}

FText UGTextInput::GetText() const
{
	return FText::AsCultureInvariant(Content->GetText());
}

const FString& UGTextInput::GetInputText()
{
	return Content->GetText();
}
void UGTextInput::SetInputText(const FString& InText)
{
	Content->SetText(InText);
}

void UGTextInput::SetPrompt(const FText& InPrompt)
{
	InputContent->SetPromptText(FUBBParser::DefaultParser.Parse(InPrompt.ToString()));
}
void UGTextInput::SetPassword(bool bInPassword)
{
	InputContent->SetPassword(bInPassword);
}

void UGTextInput::SetKeyboardType(int32 InKeyboardType) {}

void UGTextInput::SetMaxLength(int32 InMaxLength)
{
	InputContent->SetMaxLength(InMaxLength);
}

void UGTextInput::SetRestrict(const FString& InRestrict)
{
	InputContent->SetRestrict(InRestrict);
}

int32 UGTextInput::GetMaxLength()
{
	return InputContent->GetMaxLength();
}

void UGTextInput::SetBorder(int32 Border)
{
	InputContent->SetBorder(Border);
}

int32 UGTextInput::GetBorder()
{
	return InputContent->GetBorder();
}

int32 UGTextInput::GetCorner()
{
	return InputContent->GetCorner();
}

void UGTextInput::SetCorner(int32 Corner)
{
	InputContent->SetCorner(Corner);
}

void UGTextInput::SetBorderColor(const FColor& color)
{
	InputContent->SetBorderColor(color);
}

FColor UGTextInput::GetBorderColor()
{
	return InputContent->GetBorderColor();
}

void UGTextInput::SetBackgroundColor(const FColor& color)
{
	InputContent->SetBackgroundColor(color);
}

FColor UGTextInput::GetBackgroundColor()
{
	return InputContent->GetBackgroundColor();
}
bool UGTextInput::IsEditable()
{
	return InputContent->IsEditable();
}

void UGTextInput::SetEditable(bool bEditable)
{
	InputContent->SetEditable(bEditable);
}

bool UGTextInput::IsIMEEnabled()
{
	return InputContent->IsIMEEnabled();
}
void UGTextInput::EnableIME(bool bEnable)
{
	InputContent->EnableIME(bEnable);
}

bool UGTextInput::IsMouseWheelEnabled()
{
	return InputContent->IsMouseWheelEnabled();
}
void UGTextInput::EnableMouseWheel(bool bEnable)
{
	InputContent->EnableMouseWheel(bEnable);
}

void UGTextInput::ClearSelection()
{
	InputContent->ClearSelection();
}
void UGTextInput::ReplaceSelection(const FString& Value)
{
	InputContent->ReplaceSelection(Value);
}
FString UGTextInput::GetSelection()
{
	return InputContent->GetSelection();
}
void UGTextInput::SetSelection(int32 Start, int32 Length)
{
	InputContent->SetSelection(Start, Length);
}

bool UGTextInput::IsWantReturn() const
{
	return InputContent->IsWantReturn();
}
void UGTextInput::SetWantReturn(bool bWant)
{
	InputContent->SetWantReturn(bWant);
}

bool UGTextInput::IsWantTab() const
{
	return InputContent->IsWantTab();
}
void UGTextInput::SetWantTab(bool bWant)
{
	InputContent->SetWantTab(bWant);
}

bool UGTextInput::IsAutoSubmitOnLostFocus() const
{
	return InputContent->IsAutoSubmitOnLostFocus();
}
void UGTextInput::SetAutoSubmitOnLostFocus(bool bEnable)
{
	InputContent->SetAutoSubmitOnLostFocus(bEnable);
}

void UGTextInput::SetupBeforeAdd(FByteBuffer* Buffer, int32 BeginPos)
{
	UGRichTextField::SetupBeforeAdd(Buffer, BeginPos);

	Buffer->Seek(BeginPos, 4);

	const FString* str;
	if ((str = Buffer->ReadSP()) != nullptr)
	{
		FString Key = GetLocalizationKey(ID, UserData.AsString());
		auto	txt = ToLocText(*GetPackageName(), *FString::Printf(TEXT("%s_text"), *Key), **str);
		SetPrompt(txt);
	}

	if ((str = Buffer->ReadSP()) != nullptr)
		SetRestrict(*str);

	int32 iv = Buffer->ReadInt();
	if (iv != 0)
		SetMaxLength(iv);
	iv = Buffer->ReadInt();
	if (iv != 0)
		SetKeyboardType(iv);
	if (Buffer->ReadBool())
		SetPassword(true);
	InputContent->SetWantReturn(!InputContent->IsSingleLine());
}

void UGTextInput::SetupAfterAdd(FByteBuffer* Buffer, int32 BeginPos)
{
	UGRichTextField::SetupAfterAdd(Buffer, BeginPos);

	Buffer->Seek(BeginPos, 6);

	auto& str = Buffer->ReadS();
	if (!str.IsEmpty())
	{
		FString Key = GetLocalizationKey(ID, UserData.AsString());
		auto	txt = ToLocText(*GetPackageName(), *FString::Printf(TEXT("%s_text"), *Key), *str);
		SetText(txt);
	}
}
