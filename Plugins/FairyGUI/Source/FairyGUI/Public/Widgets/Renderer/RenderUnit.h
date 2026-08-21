#pragma once

#include "CoreMinimal.h"
#include "Slate.h"
#include "Widgets/Mesh/SubMesh.h"
#include "Widgets/Mesh/TextSubMesh.h"
#include "Textures/TextureAtlas.h"
#include "Utils/IDStorage.h"

enum class ERenderUnitType : uint8
{
	Unit,
	Group
};

class FSlateShaderResource;

struct FRenderUnit
{
	ERenderUnitType	   Type;
	EFairyRendererType RendererType = EFairyRendererType::Textured;
	int32			   Layer = 0; // 排序辅助：AABB 重叠层号（仅 SortChildrenUnits 内有效）

	uint32 PoolID;

	FMatrix44f LocalToWorld;
	FBox2D	   WorldBounds;

	FTextureResource*	  Texture;
	FSlateShaderResource* FontTexture;

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex>	 Triangles;
	float				 Alpha;
	float				 Saturation;
	EFairyBlendMode			 BlendMode = EFairyBlendMode::Normal;
	ETextDrawType		 TextDrawType = ETextDrawType::AlphaBitmap;
	float				 SdfEmOuterSpread = 0.0f;
	float				 SdfPixelSpread = 0.0f;
	float				 SdfBias = 0.5f;
	FVector4f			 OutlineColor = FVector4f(0, 0, 0, 0);
	float				 OutlineBias = 0.5f;

	FRenderUnit()
	{
		Type = ERenderUnitType::Unit;
		Clear();
	}
	// virtual ~FRenderUnit() = default;

	void Clear()
	{
		PoolID = 0;
		LocalToWorld = FMatrix44f::Identity;
		WorldBounds.Init();
		Texture = nullptr;
		Vertices.Reset();
		Triangles.Reset();
		Alpha = 1.0f;
		Saturation = 1.0f;
		BlendMode = EFairyBlendMode::Normal;
		TextDrawType = ETextDrawType::AlphaBitmap;
		SdfEmOuterSpread = 0.0f;
		SdfPixelSpread = 0.0f;
		SdfBias = 0.5f;
		OutlineColor = FVector4f(0, 0, 0, 0);
		OutlineBias = 0.5f;
	}
	void SetID(uint32 ID) { PoolID = ID; }

	static FRenderUnit* FromSubMesh(const TSharedPtr<FSubMesh>& SubMesh)
	{
		FRenderUnit* pUnit = FRenderUnit::Borrow();
		if (!pUnit)
			return nullptr;
		pUnit->RendererType = SubMesh->RendererType;
		pUnit->Texture = SubMesh->Texture;
		if (SubMesh->RendererType == EFairyRendererType::Text)
			pUnit->FontTexture = ((const FTextSubMesh*)(SubMesh.Get()))->FontTexture;

		// NoTexture 不需要设置纹理

		pUnit->Vertices = SubMesh->Vertices;
		pUnit->Triangles = SubMesh->Triangles;
		pUnit->LocalToWorld = SubMesh->LocalToWorld;
		pUnit->Alpha = SubMesh->Alpha;
		pUnit->Saturation = SubMesh->Saturation;
		pUnit->BlendMode = SubMesh->BlendMode;
		if (SubMesh->RendererType == EFairyRendererType::Text)
		{
			TSharedPtr<FTextSubMesh> TextSubMesh = StaticCastSharedPtr<FTextSubMesh>(SubMesh);
			pUnit->TextDrawType = TextSubMesh->DrawType;
			pUnit->SdfEmOuterSpread = TextSubMesh->SdfEmOuterSpread;
			pUnit->SdfPixelSpread = TextSubMesh->SdfPixelSpread;
			pUnit->SdfBias = TextSubMesh->SdfBias;
			pUnit->OutlineColor = FVector4f(TextSubMesh->OutlineColor.R / 255.0f, TextSubMesh->OutlineColor.G / 255.0f,
				TextSubMesh->OutlineColor.B / 255.0f, TextSubMesh->OutlineColor.A / 255.0f);
			pUnit->OutlineBias = TextSubMesh->OutlineBias;
		}

		// 将 LocalBounds 四角变换到世界空间得到 WorldBounds
		FVector2D Corners[4] = {
			FVector2D(SubMesh->LocalBounds.Min.X, SubMesh->LocalBounds.Min.Y),
			FVector2D(SubMesh->LocalBounds.Max.X, SubMesh->LocalBounds.Min.Y),
			FVector2D(SubMesh->LocalBounds.Min.X, SubMesh->LocalBounds.Max.Y),
			FVector2D(SubMesh->LocalBounds.Max.X, SubMesh->LocalBounds.Max.Y),
		};
		pUnit->WorldBounds = FBox2D(ForceInit);
		for (const FVector2D& C : Corners)
		{
			FVector4f WP = pUnit->LocalToWorld.TransformPosition(FVector4f(C.X, C.Y, 0, 1));
			pUnit->WorldBounds += FVector2D(WP.X, WP.Y);
		}

		return pUnit;
	}

	static CIDStorage<FRenderUnit> Pool;

	static FRenderUnit* Borrow();
	static void			Return(FRenderUnit* pValue);
};
