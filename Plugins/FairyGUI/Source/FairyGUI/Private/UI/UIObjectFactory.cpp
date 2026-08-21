#include "UI/UIObjectFactory.h"
#include "UI/UIPackage.h"
#include "UI/PackageItem.h"
#include "UI/GComponent.h"
#include "UI/GImage.h"
#include "UI/GMovieClip.h"
#include "UI/GTextField.h"
#include "UI/GRichTextField.h"
#include "UI/GTextInput.h"
#include "UI/GGraph.h"
#include "UI/GLoader.h"
#include "UI/GLoader3D.h"
#include "UI/GGroup.h"
#include "UI/GLabel.h"
#include "UI/GButton.h"
#include "UI/GComboBox.h"
#include "UI/GProgressBar.h"
#include "UI/GSlider.h"
#include "UI/GScrollBar.h"
#include "UI/GList.h"
#include "UI/GTree.h"

TMap<FString, FGComponentCreator> UUIObjectFactory::PackageItemExtensions;
TSubclassOf<UGLoader>			  UUIObjectFactory::LoaderExtension;

void UUIObjectFactory::SetExtension(const FString& URL, FDynGComponentCreator Creator)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogFairyGUI, Warning, TEXT("Invaild url: %s"), *URL);
		return;
	}
	FGComponentCreator& dele = PackageItemExtensions.FindOrAdd(URL);
	dele = FGComponentCreator::CreateLambda([Creator](UObject* Outer) { return Creator.Execute(Outer); });

	TSharedPtr<FPackageItem> PackageItem = UUIPackage::GetItemByURL(URL);
	if (PackageItem.IsValid())
		PackageItem->ExtensionCreator = dele;
}

void UUIObjectFactory::SetExtensionWithClass(const FString& URL, TSubclassOf<UGComponent> ClassType)
{
	if (URL.IsEmpty())
	{
		UE_LOG(LogFairyGUI, Warning, TEXT("Invaild url: %s"), *URL);
		return;
	}
	FGComponentCreator& dele = PackageItemExtensions.FindOrAdd(URL);
	dele = FGComponentCreator::CreateLambda(
		[ClassType](UObject* Outer) { return ::NewObject<UGComponent>(Outer, ClassType); });

	TSharedPtr<FPackageItem> PackageItem = UUIPackage::GetItemByURL(URL);
	if (PackageItem.IsValid())
		PackageItem->ExtensionCreator = dele;
}

UGObject* UUIObjectFactory::NewObject(const TSharedPtr<FPackageItem>& PackageItem, UObject* Outer, TSubclassOf<UGObject> ObjClass)
{
	UGObject* obj = nullptr;
	if (PackageItem->ExtensionCreator.IsBound())
		obj = PackageItem->ExtensionCreator.Execute(Outer);
	else
		obj = NewObject(PackageItem->ObjectType, Outer, ObjClass);
	if (obj != nullptr)
		obj->PackageItem = PackageItem;

	return obj;
}

UGObject* UUIObjectFactory::NewObject(EObjectType Type, UObject* Outer, TSubclassOf<UGObject> ObjClass)
{
	// 指定了ObjClass时校验兼容性：ObjClass须为当前Type默认类或其基类
	if (ObjClass != nullptr)
	{
		UClass* DefaultClass = GetClass(Type);
		if (DefaultClass == nullptr || !ObjClass->IsChildOf(DefaultClass))
			return nullptr;
		return ::NewObject<UGObject>(Outer, ObjClass);
	}

	// Loader可配置扩展加载器
	if (Type == EObjectType::Loader && LoaderExtension != nullptr)
		return ::NewObject<UGObject>(Outer, LoaderExtension);

	UClass* Cls = GetClass(Type);
	if (Cls == nullptr)
		return nullptr;
	return ::NewObject<UGObject>(Outer, Cls);
}

UClass* UUIObjectFactory::GetClass(EObjectType Type)
{
	switch (Type)
	{
		case EObjectType::Image:
			return UGImage::StaticClass();

		case EObjectType::MovieClip:
			return UGMovieClip::StaticClass();

		case EObjectType::Component:
			return UGComponent::StaticClass();

		case EObjectType::Text:
			return UGTextField::StaticClass();

		case EObjectType::RichText:
			return UGRichTextField::StaticClass();

		case EObjectType::InputText:
			return UGTextInput::StaticClass();

		case EObjectType::Group:
			return UGGroup::StaticClass();

		case EObjectType::List:
			return UGList::StaticClass();

		case EObjectType::Graph:
			return UGGraph::StaticClass();

		case EObjectType::Loader:
			return UGLoader::StaticClass();

		case EObjectType::Button:
			return UGButton::StaticClass();

		case EObjectType::Label:
			return UGLabel::StaticClass();

		case EObjectType::ProgressBar:
			return UGProgressBar::StaticClass();

		case EObjectType::Slider:
			return UGSlider::StaticClass();

		case EObjectType::ScrollBar:
			return UGScrollBar::StaticClass();

		case EObjectType::ComboBox:
			return UGComboBox::StaticClass();

		case EObjectType::Tree:
			return UGTree::StaticClass();

		case EObjectType::Loader3D:
			return UGLoader3D::StaticClass();

		default:
			return nullptr;
	}
}

void UUIObjectFactory::ResolvePackageItemExtension(const TSharedPtr<FPackageItem>& PackageItem)
{
	auto it = PackageItemExtensions.Find(TEXT("ui://") + PackageItem->Owner->GetID() + PackageItem->ID);
	if (it != nullptr)
	{
		PackageItem->ExtensionCreator = *it;
		return;
	}
	it = PackageItemExtensions.Find(TEXT("ui://") + PackageItem->Owner->GetName() + TEXT("/") + PackageItem->Name);
	if (it != nullptr)
	{
		PackageItem->ExtensionCreator = *it;
		return;
	}
	PackageItem->ExtensionCreator = nullptr;
}
