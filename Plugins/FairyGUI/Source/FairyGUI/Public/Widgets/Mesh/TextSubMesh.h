#pragma once

#include "Slate.h"
#include "Widgets/Mesh/SubMesh.h"
#include "Textures/TextureAtlas.h"

enum ETextDrawType
{
	AlphaBitmap,
	ColorBitmap,
	Line,
	SDF,
	MSDF,
	SDFLine,
};

struct FTextSubMeshKey
{
	int32			  TextureIndex;
	FTextureResource* Texture;
	ETextDrawType	  DrawType;
	FTextSubMeshKey(int32 TexIndex, FTextureResource* Tex, ETextDrawType Type)
	{
		TextureIndex = TexIndex;
		Texture = Tex;
		DrawType = Type;
	}

	bool operator==(const FTextSubMeshKey& Other) const
	{
		return TextureIndex == Other.TextureIndex && Texture == Other.Texture && DrawType == Other.DrawType;
	}
};

inline uint32 GetTypeHash(const FTextSubMeshKey& Key)
{
	uint32 Hash = GetTypeHash(Key.TextureIndex);
	Hash = HashCombine(Hash, PointerHash(Key.Texture));
	Hash = HashCombine(Hash, GetTypeHash(Key.DrawType));
	return Hash;
}

struct FTextSubMesh : public FSubMesh
{
	int32				  TextureIndex = -1;
	FSlateShaderResource* FontTexture;
	ETextDrawType		  DrawType = ETextDrawType::AlphaBitmap;
	/** SDF 外扩散量（em 单位），非 SDF 时为 0 */
	float SdfEmOuterSpread = 0.0f;
	/** SDF 内扩散量（em 单位），非 SDF 时为 0 */
	float SdfEmInnerSpread = 0.0f;
	/** SDF unitRange = 0.5 * TotalSpread * SdfPpem * InvTextureSize，对齐 UE ShaderParams.xy */
	float SdfPixelSpread = 0.0f;
	/** SDF 距离场边缘阈值，正常文字为 0.5，描边/粗体时调整 */
	float SdfBias = 0.5f;
	/** 描边颜色（a=0 表示无描边），SDF 单 pass 描边使用 */
	FColor OutlineColor = FColor::Transparent;
	/** 描边距离场阈值（低于 SdfBias，产生外观扩效果） */
	float OutlineBias = 0.5f;

	void Clear()
	{
		FSubMesh::Clear();
		TextureIndex = -1;
		FontTexture = nullptr;
		DrawType = ETextDrawType::AlphaBitmap;
		SdfEmOuterSpread = 0.0f;
		SdfEmInnerSpread = 0.0f;
		SdfPixelSpread = 0.0f;
		SdfBias = 0.5f;
		OutlineColor = FColor::Transparent;
		OutlineBias = 0.5f;
	}
};