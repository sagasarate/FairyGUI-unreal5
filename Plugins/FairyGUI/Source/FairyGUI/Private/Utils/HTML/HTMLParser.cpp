#include "Utils/HTML/HTMLParser.h"
#include "Utils/HTML/XMLIterator.h"

enum class SupportedTagNames
{
	INVALID,
	B,
	I,
	U,
	STRIKE,
	SUB,
	SUP,
	FONT,
	BR,
	IMG,
	A,
	INPUT,
	SELECT,
	P,
	UI,
	DIV,
	LI,
	HTML,
	BODY,
	HEAD,
	STYLE,
	SCRIPT,
	FORM
};

FHTMLParser		  FHTMLParser::DefaultParser;
FHTMLParseOptions FHTMLParser::DefaultParseOptions;

FHTMLParser::FHTMLParser() {}

void FHTMLParser::Parse(const FString& InText, const FNTextFormat& InFormat, TArray<FHTMLElement*>& OutElements,
	const FHTMLParseOptions& InParseOptions)
{
	ParseOptions = InParseOptions;
	Elements = &OutElements;
	TextFormatStack.Reset();
	(FNTextFormat&)Format = InFormat;
	Format.bColorChanged = false;
	int32	skipText = 0;
	bool	ignoreWhiteSpace = ParseOptions.bIgnoreWhiteSpace;
	bool	skipNextCR = false;
	FString text;

	static const TMap<FString, SupportedTagNames> TagConstMap = {
		{ TEXT("b"), SupportedTagNames::B },
		{ TEXT("i"), SupportedTagNames::I },
		{ TEXT("u"), SupportedTagNames::U },
		{ TEXT("strike"), SupportedTagNames::STRIKE },
		{ TEXT("sub"), SupportedTagNames::SUB },
		{ TEXT("sup"), SupportedTagNames::SUP },
		{ TEXT("font"), SupportedTagNames::FONT },
		{ TEXT("br"), SupportedTagNames::BR },
		{ TEXT("img"), SupportedTagNames::IMG },
		{ TEXT("a"), SupportedTagNames::A },
		{ TEXT("input"), SupportedTagNames::INPUT },
		{ TEXT("select"), SupportedTagNames::SELECT },
		{ TEXT("p"), SupportedTagNames::P },
		{ TEXT("ui"), SupportedTagNames::UI },
		{ TEXT("div"), SupportedTagNames::DIV },
		{ TEXT("li"), SupportedTagNames::LI },
		{ TEXT("html"), SupportedTagNames::HTML },
		{ TEXT("body"), SupportedTagNames::BODY },
		{ TEXT("head"), SupportedTagNames::HEAD },
		{ TEXT("style"), SupportedTagNames::STYLE },
		{ TEXT("script"), SupportedTagNames::SCRIPT },
		{ TEXT("form"), SupportedTagNames::FORM },
	};

	FXMLIterator XMLIterator;
	XMLIterator.Begin(InText, true);
	while (XMLIterator.NextTag())
	{
		if (skipText == 0)
		{
			int32 textCharIndex = XMLIterator.GetTextBeginPos(ignoreWhiteSpace);
			text = XMLIterator.GetText(ignoreWhiteSpace);
			if (text.Len() > 0)
			{
				if (skipNextCR && text[0] == TEXT('\n'))
				{
					text = text.Mid(1);
					textCharIndex++;
				}
				AppendText(text, textCharIndex);
			}
		}

		skipNextCR = false;
		switch (TagConstMap.FindRef(XMLIterator.TagName))
		{
			case SupportedTagNames::B:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					Format.bBold = true;
				}
				else
					PopTextFormat();
				break;

			case SupportedTagNames::I:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					Format.bItalic = true;
				}
				else
					PopTextFormat();
				break;

			case SupportedTagNames::U:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					Format.bUnderline = true;
				}
				else
					PopTextFormat();
				break;

			case SupportedTagNames::STRIKE:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					// Format.strikethrough = true;
				}
				else
					PopTextFormat();
				break;

			case SupportedTagNames::SUB:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					Format.SpecialStyle = SpecialStyle::Subscript;
				}
				else
					PopTextFormat();
				break;

			case SupportedTagNames::SUP:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					Format.SpecialStyle = SpecialStyle::Superscript;
				}
				else
					PopTextFormat();
				break;

			case SupportedTagNames::FONT:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();

					XMLIterator.ParseAttributes();
					Format.Size = XMLIterator.Attributes.GetInt(TEXT("size"), Format.Size);
					const FString& color = XMLIterator.Attributes.Get(TEXT("color"));
					if (color.Len() > 0)
					{
						Format.Color = FColor::FromHex(color);
						Format.bColorChanged = true;
					}
				}
				else if (XMLIterator.TagType == EXMLTagType::End)
					PopTextFormat();
				break;

			case SupportedTagNames::BR:
				AppendText(TEXT("\n"), XMLIterator.GetTagPos());
				break;

			case SupportedTagNames::IMG:
				if (XMLIterator.TagType == EXMLTagType::Start || XMLIterator.TagType == EXMLTagType::Void)
				{
					XMLIterator.ParseAttributes();

					FHTMLElement* pElement = FHTMLElement::Borrow();
					pElement->SetType(EHTMLElementType::Image);
					pElement->SetCharIndex(XMLIterator.GetTagPos());
					pElement->GetAttributes().Append(XMLIterator.Attributes);
					pElement->SetName(pElement->GetAttributes().Get(TEXT("name")));
					pElement->GetFormat().Align = Format.Align;
					Elements->Add(pElement);
				}
				break;

			case SupportedTagNames::A:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();

					Format.bUnderline = Format.bUnderline || ParseOptions.bLinkUnderline;
					if (!Format.bColorChanged && ParseOptions.LinkColor.A != 0)
						Format.Color = ParseOptions.LinkColor;

					FHTMLElement* pElement = FHTMLElement::Borrow();
					pElement->SetType(EHTMLElementType::Link);
					pElement->SetCharIndex(XMLIterator.GetTagPos());
					XMLIterator.ParseAttributes();
					pElement->GetAttributes().Append(XMLIterator.Attributes);
					pElement->SetName(pElement->GetAttributes().Get(TEXT("name")));
					pElement->GetFormat().Align = Format.Align;
					Elements->Add(pElement);
				}
				else if (XMLIterator.TagType == EXMLTagType::End)
				{
					PopTextFormat();

					FHTMLElement* pElement = FHTMLElement::Borrow();
					pElement->SetType(EHTMLElementType::LinkEnd);
					pElement->SetCharIndex(XMLIterator.GetTagPos());
					Elements->Add(pElement);
				}
				break;

			case SupportedTagNames::INPUT:
			{
				FHTMLElement* pElement = FHTMLElement::Borrow();
				pElement->SetType(EHTMLElementType::Input);
				pElement->SetCharIndex(XMLIterator.GetTagPos());
				XMLIterator.ParseAttributes();
				pElement->GetAttributes().Append(XMLIterator.Attributes);
				pElement->SetName(pElement->GetAttributes().Get(TEXT("name")));
				pElement->GetFormat() = Format;
				Elements->Add(pElement);
			}
			break;

			case SupportedTagNames::SELECT:
			{
				if (XMLIterator.TagType == EXMLTagType::Start || XMLIterator.TagType == EXMLTagType::Void)
				{
					FHTMLElement* pElement = FHTMLElement::Borrow();
					pElement->SetType(EHTMLElementType::Select);
					int32 selectCharIndex = XMLIterator.GetTagPos();
					XMLIterator.ParseAttributes();
					if (XMLIterator.TagType == EXMLTagType::Start)
					{
						FString Items, Values;
						while (XMLIterator.NextTag())
						{
							if (XMLIterator.TagName == TEXT("select"))
								break;

							if (XMLIterator.TagName == TEXT("option"))
							{
								if (XMLIterator.TagType == EXMLTagType::Start
									|| XMLIterator.TagType == EXMLTagType::Void)
								{
									if (!Values.IsEmpty())
										Values.AppendChar(TEXT(','));
									Values.Append(XMLIterator.Attributes.Get(TEXT("value")));
								}
								else
								{
									if (!Items.IsEmpty())
										Items.AppendChar(TEXT(','));
									Items.Append(XMLIterator.GetText());
								}
							}
						}
						pElement->GetAttributes().Add(TEXT("items"), Items);
						pElement->GetAttributes().Add(TEXT("values"), Values);
					}
					pElement->SetName(pElement->GetAttributes().Get(TEXT("name")));
					pElement->SetCharIndex(selectCharIndex);
					pElement->GetFormat() = Format;
					Elements->Add(pElement);
				}
			}
			break;

			case SupportedTagNames::P:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					PushTextFormat();
					const FString& align = XMLIterator.Attributes.Get(TEXT("align"));
					if (align == TEXT("center"))
						Format.Align = EAlignType::Center;
					else if (align == TEXT("right"))
						Format.Align = EAlignType::Right;

					if (!IsNewLine())
						AppendText(TEXT("\n"), XMLIterator.GetTagPos());
				}
				else if (XMLIterator.TagType == EXMLTagType::End)
				{
					AppendText(TEXT("\n"), XMLIterator.GetTagPos());
					skipNextCR = true;

					PopTextFormat();
				}
				break;

			case SupportedTagNames::UI:
			case SupportedTagNames::DIV:
			case SupportedTagNames::LI:
				if (XMLIterator.TagType == EXMLTagType::Start)
				{
					if (!IsNewLine())
						AppendText(TEXT("\n"), XMLIterator.GetTagPos());
				}
				else
				{
					AppendText(TEXT("\n"), XMLIterator.GetTagPos());
					skipNextCR = true;
				}
				break;

			case SupportedTagNames::HTML:
			case SupportedTagNames::BODY:
				// full html
				ignoreWhiteSpace = true;
				break;

			case SupportedTagNames::HEAD:
			case SupportedTagNames::STYLE:
			case SupportedTagNames::SCRIPT:
			case SupportedTagNames::FORM:
				if (XMLIterator.TagType == EXMLTagType::Start)
					skipText++;
				else if (XMLIterator.TagType == EXMLTagType::End)
					skipText--;
				break;
		}
	}

	if (skipText == 0)
	{
		int32 textCharIndex = XMLIterator.GetTextBeginPos(ignoreWhiteSpace);
		text = XMLIterator.GetText(ignoreWhiteSpace);
		if (text.Len() > 0)
		{
			if (skipNextCR && text[0] == TEXT('\n'))
			{
				text = text.Mid(1);
				textCharIndex++;
			}
			AppendText(text, textCharIndex);
		}
	}
}

void FHTMLParser::PushTextFormat()
{
	TextFormatStack.Add(Format);
}

void FHTMLParser::PopTextFormat()
{
	// 与Unity版本对齐：添加空栈保护
	if (TextFormatStack.Num() > 0)
	{
		Format = TextFormatStack.Pop();
	}
}

bool FHTMLParser::IsNewLine()
{
	if (Elements->Num() > 0)
	{
		const FHTMLElement* pElement = Elements->Last();
		if (pElement->GetType() == EHTMLElementType::Text)
			return pElement->GetText().EndsWith(TEXT("\n"));
		else
			return false;
	}

	return true;
}

void FHTMLParser::AppendText(const FString& Text, int32 CharIndex)
{
	if (Elements->Num() > 0)
	{
		FHTMLElement* pElement = Elements->Last();
		if (pElement->GetType() == EHTMLElementType::Text && pElement->GetFormat().EqualStyle(Format))
		{
			pElement->GetText().Append(Text);
			return;
		}
	}

	{
		FHTMLElement* pElement = FHTMLElement::Borrow();
		pElement->SetType(EHTMLElementType::Text);
		pElement->SetCharIndex(CharIndex);
		pElement->GetText() = Text;
		pElement->GetFormat() = Format;
		Elements->Add(pElement);
	}
}