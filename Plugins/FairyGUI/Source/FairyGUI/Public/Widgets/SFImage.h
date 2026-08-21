#pragma once

#include "SDisplayObject.h"
#include "UI/FieldTypes.h"
#include "Mesh/FillMesh.h"
#include "NTexture.h"
#include "Utils/TypeID.h"

class FAIRYGUI_API SFImage : public SDisplayObject, public FGCObject
{
protected:
	TOptional<FBox2D> m_Scale9Grid;
	bool			  m_bScaleByTile = false;
	FVector2D		  m_TextureScale;
	int32			  m_TileGridIndice = 0;
	EFlipType		  m_Flip = EFlipType::None;

	TUniquePtr<FFillMesh> m_FillMesh;

	mutable TObjectPtr<UNTexture> m_Texture;

	mutable TSharedPtr<FSubMesh> m_pSubMesh;

public:
	DECLARE_TYPE_ID(SFImage, SDisplayObject)

	explicit SFImage(UGObject* InGObject = nullptr);

	void	   SetTexture(UNTexture* InTexture);
	UNTexture* GetTexture() const { return m_Texture; }
	void	   SetNativeSize();
	void	   SetScale9Grid(const TOptional<FBox2D>& GridRect);
	void	   SetScaleByTile(bool bInScaleByTile);
	void	   SetTileGridIndice(int32 InTileGridIndex);

	EFillMethod GetFillMethod() const;
	void		SetFillMethod(EFillMethod Method);
	int32		GetFillOrigin() const;
	void		SetFillOrigin(int32 Origin);
	bool		IsFillClockwise() const;
	void		SetFillClockwise(bool bClockwise);
	float		GetFillAmount() const;
	void		SetFillAmount(float Amount);

	EFlipType GetFlip() const { return m_Flip; }
	void	  SetFlip(EFlipType Value);

	void  SetSaturation(float InSaturation) { m_Saturation = InSaturation; }
	float GetSaturation() const { return m_Saturation; }

public:
	virtual void CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
		float InSaturation) const override;

	virtual void	AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

protected:
	void RebuildMesh(int32 LayerID);

	void TileFill(FSubMesh* pSubMesh, const FBox2D& ContentRect, const FBox2D& UVRect, const FVector2D& TextureSize);
	void SliceFill(FSubMesh* pSubMesh);
};