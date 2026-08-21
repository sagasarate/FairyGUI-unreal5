#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

struct FAIRYGUI_API FUIEvents
{
    static const FName Click;
    static const FName TouchBegin;
    static const FName TouchMove;
    static const FName TouchEnd;
    static const FName RollOver;
    static const FName RollOut;
    static const FName MouseWheel;

    static const FName DragStart;
    static const FName DragMove;
    static const FName DragEnd;
    static const FName Drop;

    static const FName AddedToStage;
    static const FName RemovedFromStage;

    static const FName Changed;
    static const FName ClickItem;
    static const FName ClickLink;
    static const FName GearStop;

    static const FName Scroll;
    static const FName ScrollEnd;
    static const FName PullDownRelease;
	static const FName PullUpRelease;

	static const FName KeyDown;
	static const FName KeyUp;

	static const FName Submit;
    static const FName Cancel;
    static const FName GetFocus;
    static const FName LostFocus;

	static const FName HtmlButtonClick;
	static const FName HtmlLinkClick;
	static const FName HtmlLinkRollOver;
	static const FName HtmlLinkRollOut;
	static const FName HtmlSelectChanged;
};