#pragma once

#include "Utils/HTML/XMLAttributes.h"

enum class EXMLTagType
{
    Start,
    End,
    Void,
    CDATA,
    Comment,
    Instruction
};

struct FAIRYGUI_API FXMLIterator
{
public:
    void Begin(const FString& InText, bool bLowerCaseName = false);
    bool NextTag();

    FString GetTagSource() const;
    FString GetRawText(bool bTrim = false) const;
    FString GetText(bool bTrim = false) const;
    int32  GetTextBeginPos(bool bTrim = false) const;
    void ParseAttributes();

    int32 GetLastTagEnd() const { return LastTagEnd; }
    int32 GetTagPos() const { return TagPos; }

    static FString DecodeString(const FString& InSource);

    FString TagName;
    EXMLTagType TagType;
    FString LastTagName;
    FXMLAttributes Attributes;

private:

    const FString* Source;
    int32 SourceLen;
    int32 ParsePos;
    int32 TagPos;
    int32 TagLength;
    int32 LastTagEnd;
    bool bAttrParsed;
    bool bLowerCaseName;
};