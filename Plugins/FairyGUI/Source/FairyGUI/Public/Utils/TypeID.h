#pragma once

#include "CoreMinimal.h"

#define DECLARE_TYPE_ID(ClassType, ParentClassType)                                 \
	static FName  s_TypeName;                                                       \
	virtual FName GetTypeName() const override                                      \
	{                                                                               \
		return s_TypeName;                                                          \
	}                                                                               \
	static FName GetTypeNameStatic();                                               \
	virtual bool IsTypeOf(FName TypeName) const override                            \
	{                                                                               \
		return s_TypeName == TypeName ? true : ParentClassType::IsTypeOf(TypeName); \
	}

#define IMPLEMENT_TYPE_ID(ClassType)               \
	FName ClassType::s_TypeName(TEXT(#ClassType)); \
	FName ClassType::GetTypeNameStatic()           \
	{                                              \
		return s_TypeName;                         \
	}

template <class T1, class T2> inline T1* CastObject(T2* pObj)
{
	if (pObj)
	{
		if (pObj->IsTypeOf(T1::GetTypeNameStatic()))
			return static_cast<T1*>(pObj);
	}
	return nullptr;
}