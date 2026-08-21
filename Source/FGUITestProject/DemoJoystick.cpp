#include "DemoJoystick.h"

void UDemoJoystick::OnDemoStart_Implementation()
{
	UUIPackage::AddPackage(TEXT("/Game/UI/Joystick"), this);

	MainView = UUIPackage::CreateObject(TEXT("Joystick"), TEXT("Main"), this)->As<UGComponent>();
	MainView->MakeFullScreen();
	MainView->SetParentToRoot();

	Joystick = MakeShared<FJoystickModule>(MainView);

	UGObject* TextField = MainView->GetChild(TEXT("n9"));

	Joystick->OnMoving.BindLambda([TextField](float Degree) {
		TextField->SetText(FText::AsNumber(Degree));
	});

	Joystick->OnMoveEnd.BindLambda([TextField]() {
		TextField->SetText(G_EMPTY_TEXT);
	});
}

void UDemoJoystick::OnDemoEnd_Implementation()
{
	UUIPackage::RemovePackage(TEXT("Joystick"), this);
}