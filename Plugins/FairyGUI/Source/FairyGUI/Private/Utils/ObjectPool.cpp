#include "Utils/ObjectPool.h"
#include "UI/UIObjectFactory.h"
#include "UI/GObject.h"
#include "UI/PackageItem.h"
#include "UI/UIPackage.h"

UObject* UObjectPool::Borrow(UClass* Class, bool bHold)
{
	UObject* Object = nullptr;
	auto	 ObjList = ObjectMap.Find(Class);
	if (ObjList)
	{
		if (ObjList->ObjectList.Num() > 0)
		{
			Object = ObjList->ObjectList.Pop();
		}
	}
	if (!Object)
		Object = NewObject<UObject>(this, Class);
	if (bHold && Object)
		HoldObjects.Add(Object);
	return Object;
}
UObject* UObjectPool::Borrow(EObjectType Type, bool bHold)
{
	UObject* Object = nullptr;
	UClass*	 Class = UUIObjectFactory::GetClass(Type);
	auto	 ObjList = ObjectMap.Find(Class);
	if (ObjList)
	{
		if (ObjList->ObjectList.Num() > 0)
		{
			Object = ObjList->ObjectList.Pop();
		}
	}
	if (!Object)
		Object = UUIObjectFactory::NewObject(Type, this, nullptr);
	if (bHold && Object)
		HoldObjects.Add(Object);
	return Object;
}
UObject* UObjectPool::Borrow(const FString& ResURL, bool bHold)
{
	TSharedPtr<FPackageItem> pii = UUIPackage::GetItemByURL(ResURL);
	if (pii)
	{
		UGObject* Object = nullptr;
		auto	  MapByPackage = ObjectMapByRes.Find(pii->Owner->GetID());
		if (MapByPackage)
		{
			auto ObjList = MapByPackage->ObjectMap.Find(pii->ID);
			if (ObjList)
			{
				if (ObjList->ObjectList.Num() > 0)
				{
					Object = Cast<UGObject>(ObjList->ObjectList.Pop());
				}
			}
		}
		if (!Object)
			Object = pii->Owner->CreateObject(pii, this, nullptr);
		if (bHold && Object)
			HoldObjects.Add(Object);
		return Object;
	}
	return nullptr;
}
void UObjectPool::Return(UObject* Obj)
{
	if (!IsValid(Obj))
		return;
	UGObject* GObject = Cast<UGObject>(Obj);
	if (GObject)
	{
		if (auto pii = GObject->GetPackageItem())
		{
			auto MapByPackage = ObjectMapByRes.FindOrAdd(pii->Owner->GetID());
			auto ObjList = MapByPackage.ObjectMap.FindOrAdd(pii->ID);
			if (ObjList.ObjectList.Num() < m_MaxPoolObject)
				ObjList.ObjectList.Add(Obj);
			HoldObjects.Remove(Obj);
			return;
		}
	}

	auto& ObjList = ObjectMap.FindOrAdd(Obj->StaticClass());
	if (ObjList.ObjectList.Num() < m_MaxPoolObject)
		ObjList.ObjectList.Add(Obj);
	HoldObjects.Remove(Obj);
}
void UObjectPool::Return(TArray<UObject*>& Objs)
{
	for (auto Obj : Objs)
		Return(Obj);
	Objs.Empty();
}
void UObjectPool::Unhold(UObject* Obj)
{
	HoldObjects.Remove(Obj);
}
void UObjectPool::UnholdAll()
{
	HoldObjects.Empty();
}
void UObjectPool::Clear(UClass* Class)
{
	ObjectMap.Remove(Class);
}
void UObjectPool::Clear(EObjectType Type)
{
	UClass* Class = UUIObjectFactory::GetClass(Type);
	ObjectMap.Remove(Class);
}
void UObjectPool::Clear(const FString& ResURL)
{
	TSharedPtr<FPackageItem> pii = UUIPackage::GetItemByURL(ResURL);
	if (pii)
	{
		auto MapByPackage = ObjectMapByRes.Find(pii->Owner->GetID());
		if (MapByPackage)
			MapByPackage->ObjectMap.Remove(pii->ID);
	}
}
void UObjectPool::ClearByPackage(const FString& PackageID)
{
	ObjectMapByRes.Remove(PackageID);
}
void UObjectPool::ClearAll(bool bClearHold)
{
	ObjectMapByRes.Empty();
	ObjectMap.Empty();
	if (bClearHold)
		HoldObjects.Empty();
}

void UObjectPool::DisposeAllObjects()
{
	// 按类分类的池
	for (auto& Elem : ObjectMap)
	{
		for (auto& Obj : Elem.Value.ObjectList)
		{
			if (auto* GObj = Cast<UGObject>(Obj.Get()))
				GObj->Dispose();
		}
	}

	// 按资源 URL 分类的池
	for (auto& Elem : ObjectMapByRes)
	{
		for (auto& MapEntry : Elem.Value.ObjectMap)
		{
			for (auto& Obj : MapEntry.Value.ObjectList)
			{
				if (auto* GObj = Cast<UGObject>(Obj.Get()))
					GObj->Dispose();
			}
		}
	}

	// 显式持有的对象
	for (auto& Obj : HoldObjects)
	{
		if (auto* GObj = Cast<UGObject>(Obj.Get()))
			GObj->Dispose();
	}

	// 清空池
	ClearAll(true);
}