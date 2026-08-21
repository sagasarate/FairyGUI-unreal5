#pragma once

#include "CoreMinimal.h"
#include "Misc/TVariant.h"
#include <type_traits>
#include "NVariant.generated.h"

USTRUCT(BlueprintType)
struct FAIRYGUI_API FNVariant
{
	GENERATED_USTRUCT_BODY()
private:
	using TVariantType = TVariant<FEmptyVariantState, bool, int32, uint32, float, FString, FText, FColor, TWeakObjectPtr<UObject>, void*>;
	TVariantType Data;

public:
	FNVariant() = default;
	FNVariant(const FNVariant&) = default;
	FNVariant(FNVariant&&) = default;

	FNVariant& operator=(const FNVariant&) = default;
	FNVariant& operator=(FNVariant&&) = default;

	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FNVariant>>>
	explicit FNVariant(T&& InValue)
	{
		using RawType = typename TDecay<T>::Type;

		if constexpr (TIsPointer<RawType>::Value && TIsDerivedFrom<typename TRemovePointer<RawType>::Type, UObject>::Value)
		{
			Data.Set<TWeakObjectPtr<UObject>>(TWeakObjectPtr<UObject>((UObject*)InValue));
		}
		else if constexpr (std::is_pointer_v<RawType> && !std::is_same_v<RawType, UObject*>)
		{
			Data.Set<void*>((void*)InValue);
		}
		else
		{
			Data.Set<RawType>(Forward<T>(InValue));
		}
	}

	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FNVariant>>>
	FNVariant& operator=(T&& InValue)
	{
		using RawType = typename TDecay<T>::Type;

		if constexpr (TIsPointer<RawType>::Value && TIsDerivedFrom<typename TRemovePointer<RawType>::Type, UObject>::Value)
		{
			Data.Set<TWeakObjectPtr<UObject>>(TWeakObjectPtr<UObject>((UObject*)InValue));
		}
		else if constexpr (std::is_pointer_v<RawType> && !std::is_same_v<RawType, UObject*>)
		{
			Data.Set<void*>((void*)InValue);
		}
		else
		{
			Data.Set<RawType>(Forward<T>(InValue));
		}
		return *this;
	}

	bool AsBool() const
	{
		return As<bool>();
	}

	int32 AsInt() const
	{
		return As<int32>();
	}

	int32 AsUint() const
	{
		return As<uint32>();
	}

	float AsFloat() const
	{
		return As<float>();
	}

	FString AsString() const
	{
		return As<FString>();
	}

	FText AsText() const
	{
		return As<FText>();
	}

	FColor AsColor() const
	{
		return As<FColor>();
	}

	UObject* AsUObject() const
	{
		return As<UObject*>();
	}

	template <typename DataType>
	DataType As() const
	{
		if (const DataType* Ptr = Data.TryGet<DataType>())
		{
			return *Ptr;
		}
		return DataType();
	}

	template <>
	inline float As<float>() const
	{
		if (const float* Ptr = Data.TryGet<float>())
		{
			return *Ptr;
		}
		if (const int32* IntPtr = Data.TryGet<int32>())
		{
			return static_cast<float>(*IntPtr);
		}
		if (const uint32* UintPtr = Data.TryGet<uint32>())
		{
			return static_cast<float>(*UintPtr);
		}
		if (const FString* StrPtr = Data.TryGet<FString>())
		{
			return FCString::Atof(**StrPtr);
		}
		if (const FText* TxtPtr = Data.TryGet<FText>())
		{
			return FCString::Atof(*TxtPtr->ToString());
		}
		return 0.0f;
	}

	template <>
	inline int32 As<int32>() const
	{
		if (const int32* Ptr = Data.TryGet<int32>())
		{
			return *Ptr;
		}
		if (const uint32* Ptr = Data.TryGet<uint32>())
		{
			return static_cast<int32>(*Ptr);
		}
		if (const float* FloatPtr = Data.TryGet<float>())
		{
			return FMath::RoundToInt(*FloatPtr);
		}
		if (const FString* StrPtr = Data.TryGet<FString>())
		{
			return FCString::Atoi(**StrPtr);
		}
		if (const FText* TxtPtr = Data.TryGet<FText>())
		{
			return FCString::Atoi(*TxtPtr->ToString());
		}
		return 0;
	}

	template <>
	inline uint32 As<uint32>() const
	{
		if (const uint32* Ptr = Data.TryGet<uint32>())
		{
			return *Ptr;
		}
		if (const int32* Ptr = Data.TryGet<int32>())
		{
			return static_cast<uint32>(*Ptr);
		}
		if (const float* FloatPtr = Data.TryGet<float>())
		{
			return static_cast<uint32>(FMath::RoundToInt(*FloatPtr));
		}
		if (const FString* StrPtr = Data.TryGet<FString>())
		{
			return static_cast<uint32>(FCString::Atoi(**StrPtr));
		}
		if (const FText* TxtPtr = Data.TryGet<FText>())
		{
			return static_cast<uint32>(FCString::Atoi(*TxtPtr->ToString()));
		}
		return 0;
	}

	template <>
	inline FString As<FString>() const
	{
		if (const FString* Ptr = Data.TryGet<FString>())
		{
			return *Ptr;
		}
		if (const FText* Ptr = Data.TryGet<FText>())
		{
			return Ptr->ToString();
		}
		if (const float* Ptr = Data.TryGet<float>())
		{
			return FString::Printf(TEXT("%f"), *Ptr);
		}
		if (const int32* IntPtr = Data.TryGet<int32>())
		{
			return FString::Printf(TEXT("%d"), *IntPtr);
		}
		if (const uint32* UintPtr = Data.TryGet<uint32>())
		{
			return FString::Printf(TEXT("%u"), *UintPtr);
		}
		return FString();
	}

	template <>
	inline FText As<FText>() const
	{
		if (const FText* Ptr = Data.TryGet<FText>())
		{
			return *Ptr;
		}
		if (const FString* Ptr = Data.TryGet<FString>())
		{
			return FText::AsCultureInvariant(*Ptr);
		}
		if (const float* Ptr = Data.TryGet<float>())
		{
			return FText::AsNumber(*Ptr);
		}
		if (const int32* IntPtr = Data.TryGet<int32>())
		{
			return FText::AsNumber(*IntPtr);
		}
		if (const uint32* UintPtr = Data.TryGet<uint32>())
		{
			return FText::AsNumber(*UintPtr);
		}
		return FText();
	}

	template <>
	inline UObject* As<UObject*>() const
	{
		if (const TWeakObjectPtr<UObject>* Ptr = Data.TryGet<TWeakObjectPtr<UObject>>())
		{
			return Ptr->Get();
		}
		return nullptr;
	}

	void Reset()
	{
		Data.Set<FEmptyVariantState>(FEmptyVariantState());
	}

	static const FNVariant Null;
};