#pragma once

#include "Widgets/Mesh/TextSubMesh.h"

enum class ETextSubMeshType
{
	Shadow,
	Outline,
	Text,
	Max,
};

struct FTextMesh
{
	TMap<FTextSubMeshKey, TSharedPtr<FTextSubMesh>> SubMeshs[(int32)ETextSubMeshType::Max];

	void Reset()
	{
		for (auto& TypeSubMeshs : SubMeshs)
		{
			for (auto iter = TypeSubMeshs.CreateIterator(); iter; ++iter)
			{
				auto pSubMesh = iter.Value();
				pSubMesh->Reset();
			}
		}
	}
	void RemoveEmptySubMeshs()
	{
		for (auto& TypeSubMeshs : SubMeshs)
		{
			for (auto iter = TypeSubMeshs.CreateIterator(); iter; ++iter)
			{
				auto pSubMesh = iter.Value();
				if (pSubMesh->Triangles.IsEmpty())
					iter.RemoveCurrent();
			}
		}
	}
	void RemoveAllSubMeshs()
	{
		for (auto& TypeSubMeshs : SubMeshs)
			TypeSubMeshs.Reset();
	}

	int32 GetSubMeshCount() const
	{
		int32 Count = 0;
		for (auto& TypeSubMeshs : SubMeshs)
			Count += TypeSubMeshs.Num();
		return Count;
	}

	void GetSubMeshs(TArray<TSharedPtr<FTextSubMesh>>& SubMeshArray)
	{
		for (auto& TypeSubMeshs : SubMeshs)
		{
			for (auto iter = TypeSubMeshs.CreateIterator(); iter; ++iter)
				SubMeshArray.Add(iter->Value);
		}
	}

	TSharedPtr<FTextSubMesh> GetSubMesh(ETextSubMeshType Type, int32 TextureIndex, ETextDrawType DrawType)
	{
		if (Type < (ETextSubMeshType)0 || Type >= ETextSubMeshType::Max)
			return nullptr;
		auto& TypeSubMeshs = SubMeshs[(int32)Type];

		FTextSubMeshKey			 Key(TextureIndex, nullptr, DrawType);
		TSharedPtr<FTextSubMesh> pSubMesh = nullptr;
		if (TypeSubMeshs.Contains(Key))
		{
			pSubMesh = TypeSubMeshs[Key];
		}
		else
		{
			pSubMesh = MakeShared<FTextSubMesh>();
			pSubMesh->TextureIndex = TextureIndex;
			pSubMesh->DrawType = DrawType;
			TypeSubMeshs.Add(Key, pSubMesh);
		}
		return pSubMesh;
	}
	TSharedPtr<FTextSubMesh> GetSubMesh(ETextSubMeshType Type, FTextureResource* Texture, ETextDrawType DrawType)
	{
		if (Type < (ETextSubMeshType)0 || Type >= ETextSubMeshType::Max)
			return nullptr;
		auto& TypeSubMeshs = SubMeshs[(int32)Type];

		FTextSubMeshKey			 Key(-1, Texture, DrawType);
		TSharedPtr<FTextSubMesh> pSubMesh = nullptr;
		if (TypeSubMeshs.Contains(Key))
		{
			pSubMesh = TypeSubMeshs[Key];
		}
		else
		{
			pSubMesh = MakeShared<FTextSubMesh>();
			pSubMesh->Texture = Texture;
			pSubMesh->DrawType = DrawType;
			TypeSubMeshs.Add(Key, pSubMesh);
		}
		return pSubMesh;
	}
};