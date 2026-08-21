#pragma once

#include "CoreMinimal.h"
#include "Slate.h"
#include "Utils/IDStorage.h"
#include "UI/FieldTypes.h"

enum class EFairyRendererType : uint8
{
	Textured,  // FFairyRenderer — 有纹理（SFImage, SShape, SMovieClip）
	Text,	   // FFairyTextRenderer — 字体图集纹理（STextField）
	NoTexture, // FFairyNoTextureRenderer — 无纹理（SSelectionShape）
};

class FSlateShaderResource;

struct FSubMesh
{
	int32 PoolID;

	int32			  LayerID;
	FTextureResource* Texture;

	EFairyRendererType RendererType = EFairyRendererType::Textured;

	FMatrix44f LocalToWorld;
	FBox2D	   ContentRect;
	FBox2D	   UVRect;
	FBox2D	   LocalBounds; // 本地空间包围盒，AddVertex 时增量更新
	FColor	   VertexColor;

	float	   Alpha = 1.0f;
	float	   Saturation = 1.0f;
	EFairyBlendMode BlendMode = EFairyBlendMode::Normal;
	FSlateRect ScissorRect;

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex>	 Triangles;

	FSubMesh() { Clear(); }
	void Reset()
	{
		Vertices.Reset();
		Triangles.Reset();
		LocalBounds = FBox2D(ForceInit);
	}
	void Clear()
	{
		PoolID = 0;
		LayerID = 0;
		Texture = nullptr;
		RendererType = EFairyRendererType::Textured;
		LocalToWorld.SetIdentity();
		ContentRect = FBox2D(ForceInit);
		UVRect = FBox2D(ForceInit);
		VertexColor = FColor::White;
		LocalBounds = FBox2D(ForceInit);
		Alpha = 1.0f;
		Saturation = 1.0f;
		BlendMode = EFairyBlendMode::Normal;
		ScissorRect = FSlateRect(0, 0, 0, 0);
		Vertices.Empty();
		Triangles.Empty();
	}
	void  SetID(int32 ID) { PoolID = ID; }
	int32 GetVertexCount() const { return Vertices.Num(); }
	void  AddVertex(const FVector2D& Position) { AddVertex(Position, VertexColor); }
	void  AddVertex(const FVector2D& Position, const FColor& Color)
	{
		AddVertex(
			Position, Color, FMath::Lerp(UVRect.Min, UVRect.Max, (Position - ContentRect.Min) / ContentRect.GetSize()));
	}
	void AddVertex(const FVector2D& Position, const FColor& Color, const FVector2D& TexCoords)
	{
		FSlateVertex& Vertex = Vertices.AddDefaulted_GetRef();
		Vertex.Position = FVector2f(Position);
		Vertex.Color = Color;
		Vertex.TexCoords[0] = TexCoords.X;
		Vertex.TexCoords[1] = TexCoords.Y;
		Vertex.TexCoords[2] = 1;
		Vertex.TexCoords[3] = 1;
		Vertex.MaterialTexCoords[0] = TexCoords.X;
		Vertex.MaterialTexCoords[1] = TexCoords.Y;
		// UE 5.7 新增字段，FSlateVertex 是 POD 类型，AddDefaulted 不会零初始化，
		// 未初始化内存被提交到 GPU 时在大量顶点场景下会引发多边形闪烁和错误贴图。
		Vertex.SecondaryColor = FColor::Transparent;
		Vertex.PixelSize[0] = 0;
		Vertex.PixelSize[1] = 0;
		LocalBounds += Position;
	}
	void AddQuad(const FBox2D& VertRect)
	{
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Max.Y));
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Min.Y));
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Min.Y));
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Max.Y));
	}
	void AddQuad(const FBox2D& VertRect, const FColor& Color)
	{
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Max.Y), Color);
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Min.Y), Color);
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Min.Y), Color);
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Max.Y), Color);
	}

	void AddQuad(const FBox2D& VertRect, const FColor& Color, const FBox2D& InUVRect)
	{
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Max.Y), Color, FVector2D(InUVRect.Min.X, InUVRect.Max.Y));
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Min.Y), Color, FVector2D(InUVRect.Min.X, InUVRect.Min.Y));
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Min.Y), Color, FVector2D(InUVRect.Max.X, InUVRect.Min.Y));
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Max.Y), Color, FVector2D(InUVRect.Max.X, InUVRect.Max.Y));
	}

	void AddQuad(const FBox2D& VertRect, const FColor& Color, const FBox2D& InUVRect, float Skew)
	{
		AddVertex(FVector2D(VertRect.Min.X, VertRect.Max.Y), Color, FVector2D(InUVRect.Min.X, InUVRect.Max.Y));
		AddVertex(FVector2D(VertRect.Min.X + Skew, VertRect.Min.Y), Color, FVector2D(InUVRect.Min.X, InUVRect.Min.Y));
		AddVertex(FVector2D(VertRect.Max.X + Skew, VertRect.Min.Y), Color, FVector2D(InUVRect.Max.X, InUVRect.Min.Y));
		AddVertex(FVector2D(VertRect.Max.X, VertRect.Max.Y), Color, FVector2D(InUVRect.Max.X, InUVRect.Max.Y));
	}

	void RepeatColors(FColor* Colors, int32 ColorCount, int32 StartIndex, int32 Count)
	{
		if (Count < 0)
			Count = Vertices.Num() - StartIndex;
		int32 len = FMath::Min(StartIndex + Count, Vertices.Num());
		int32 k = 0;
		for (int32 i = StartIndex; i < len; i++)
		{
			Vertices[i].Color = Colors[(k++) % ColorCount];
		}
	}
	void AddTriangle(SlateIndex idx0, SlateIndex idx1, SlateIndex idx2)
	{
		Triangles.Add(idx0);
		Triangles.Add(idx1);
		Triangles.Add(idx2);
	}

	void AddTriangles(const SlateIndex* Indice, int32 IndiceLength, int32 StartVertexIndex = 0)
	{
		if (StartVertexIndex != 0)
		{
			if (StartVertexIndex < 0)
				StartVertexIndex = Vertices.Num() + StartVertexIndex;

			for (int32 i = 0; i < IndiceLength; i++)
				Triangles.Add(Indice[i] + StartVertexIndex);
		}
		else
		{
			Triangles.Append(Indice, IndiceLength);
		}
	}

	void AddTriangles(int32 StartVertexIndex = 0)
	{
		int32 cnt = Vertices.Num();
		if (StartVertexIndex < 0)
			StartVertexIndex = cnt + StartVertexIndex;

		for (int32 i = StartVertexIndex; i < cnt; i += 4)
		{
			Triangles.Add(i);
			Triangles.Add(i + 1);
			Triangles.Add(i + 2);

			Triangles.Add(i + 2);
			Triangles.Add(i + 3);
			Triangles.Add(i);
		}
	}
	const FVector2D GetPosition(int32 Index)
	{
		if (Index < 0)
			Index = Vertices.Num() + Index;

		return FVector2D(Vertices[Index].Position);
	}

	FVector2D GetUVAtPosition(const FVector2D& Position, bool bUsePercent)
	{
		if (bUsePercent)
			return FMath::Lerp(UVRect.Min, UVRect.Max, Position);
		else
			return FMath::Lerp(UVRect.Min, UVRect.Max, (Position - ContentRect.Min) / ContentRect.GetSize());
	}
};