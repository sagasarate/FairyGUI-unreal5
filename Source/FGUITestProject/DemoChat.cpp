#include "DemoChat.h"

UDemoChat::UDemoChat()
{
	TArray<FString> tags{ TEXT("88"), TEXT("am"), TEXT("bs"), TEXT("bz"), TEXT("ch"), TEXT("cool"), TEXT("dhq"),
		TEXT("dn"), TEXT("fd"), TEXT("gz"), TEXT("han"), TEXT("hx"), TEXT("hxiao"), TEXT("hxiu") };

	for (auto& str : tags)
		EmojiParser.Handlers.Add(TEXT(":") + str, FTagHandler::CreateUObject(this, &UDemoChat::OnTag_Emoji));
}

UDemoChat::~UDemoChat() {}

void UDemoChat::OnDemoStart_Implementation()
{
	UUIPackage::AddPackage(TEXT("/Game/UI/Chat"), this);

	MainView = UUIPackage::CreateObject(TEXT("Chat"), TEXT("Main"), this)->As<UGComponent>();
	MainView->MakeFullScreen();
	MainView->SetParentToRoot();

	List = MainView->GetChild(TEXT("list"))->As<UGList>();
	List->SetVirtual();
	List->SetItemProvider(FListItemProvider::CreateUObject(this, &UDemoChat::GetListItemResource));
	List->SetItemRenderer(FListItemRenderer::CreateUObject(this, &UDemoChat::RenderListItem));

	Input = MainView->GetChild(TEXT("input"))->As<UGTextInput>();
	Input->OnSubmit.AddUniqueDynamic(this, &UDemoChat::OnSubmit);

	MainView->GetChild(TEXT("btnSend"))->OnClick.AddUniqueDynamic(this, &UDemoChat::OnClickSendBtn);
	MainView->GetChild(TEXT("btnEmoji"))->OnClick.AddUniqueDynamic(this, &UDemoChat::OnClickEmojiBtn);

	EmojiSelectUI = UUIPackage::CreateObject(TEXT("Chat"), TEXT("EmojiSelectUI"), this)->As<UGComponent>();
	EmojiSelectUI->GetChild(TEXT("list"))->As<UGList>()->OnClickItem.AddUniqueDynamic(this, &UDemoChat::OnClickEmoji);
}

void UDemoChat::OnDemoEnd_Implementation()
{
	UUIPackage::RemovePackage(TEXT("Chat"), this);
}

void UDemoChat::OnClickSendBtn(UEventContext* Context)
{
	auto& msg = Input->GetInputText();
	if (msg.IsEmpty())
		return;

	AddMsg(FText::AsCultureInvariant(TEXT("UnityTEXT(")), TEXT(")r0"), msg, true);
	Input->SetInputText(G_EMPTY_STRING);
}

void UDemoChat::OnClickEmojiBtn(UEventContext* Context)
{
	MainView->GetUIRoot()->ShowPopup(EmojiSelectUI, Context->GetSender(), EPopupDirection::Up);
}

void UDemoChat::OnClickEmoji(UEventContext* Context)
{
	UGObject* item = Cast<UGObject>(Context->GetData().AsUObject());
	Input->SetText(FText::Format(FText::AsCultureInvariant(TEXT("{0}[:{1}]")), Input->GetText(), item->GetText()));
}

void UDemoChat::OnSubmit(UEventContext* Context)
{
	OnClickSendBtn(nullptr);
}

void UDemoChat::RenderListItem(int32 Index, UGObject* Obj)
{
	UGButton*	  item = Obj->As<UGButton>();
	FMessageInfo& Info = Messages[Index];
	if (!Info.bFromMe)
		item->GetChild(TEXT("name"))->SetText(Info.Sender);
	item->SetIcon(TEXT("ui://Chat/") + Info.SenderIcon);

	UGRichTextField* tf = item->GetChild(TEXT("msg"))->As<UGRichTextField>();
	tf->SetText(FText::AsCultureInvariant(EmojiParser.Parse(Info.Msg)));
}

FString UDemoChat::GetListItemResource(int32 Index)
{
	FMessageInfo Info = Messages[Index];
	if (Info.bFromMe)
		return TEXT("ui://Chat/chatRight");
	else
		return TEXT("ui://Chat/chatLeft");
}

void UDemoChat::AddMsg(const FText& Sender, const FString& SenderIcon, const FString& Msg, bool bFromMe)
{
	bool		 isScrollBottom = List->GetScrollPane()->IsBottomMost();
	FMessageInfo NewInfo;
	NewInfo.Sender = Sender;
	NewInfo.SenderIcon = SenderIcon;
	NewInfo.Msg = Msg;
	NewInfo.bFromMe = bFromMe;
	Messages.Add(NewInfo);

	if (NewInfo.bFromMe)
	{
		if (Messages.Num() == 1 || FMath::RandBool())
		{
			FMessageInfo ReplayInfo;
			ReplayInfo.Sender = FText::AsCultureInvariant(TEXT("FairyGUI"));
			ReplayInfo.SenderIcon = TEXT("r1");
			ReplayInfo.Msg = TEXT("Today is a good day. [:cool]");
			ReplayInfo.bFromMe = false;
			Messages.Add(ReplayInfo);
		}
	}

	if (Messages.Num() > 100)
		Messages.RemoveAt(0, Messages.Num() - 100);

	List->SetNumItems(Messages.Num());

	if (isScrollBottom)
		List->GetScrollPane()->ScrollBottom(true);
}

FString UDemoChat::OnTag_Emoji(const FString& TagName, bool bEnd, const FString& Attr)
{
	FString str = TagName.Mid(1);
	str.ToLowerInline();
	return TEXT("<img src='ui://Chat/") + str + TEXT("'/>");
}