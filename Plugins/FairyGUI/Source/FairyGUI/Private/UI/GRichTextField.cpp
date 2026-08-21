#include "UI/GRichTextField.h"

UGRichTextField::UGRichTextField() {}

void UGRichTextField::CreateDisplayObject()
{
	DisplayObject = Content = MakeShared<SRichTextField>(this);
}

UGRichTextField::~UGRichTextField() {}