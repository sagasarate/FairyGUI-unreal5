#pragma once

#include "SDisplayObject.h"
#include "NTexture.h"
#include "Utils/TypeID.h"
#include "Widgets/Mesh/SubMesh.h"

UENUM()
enum class EFGUIShapeType : uint8
{
	None,
	Rect,
	RoundRect,
	Ellipse,
	Polygon,
	RegularPolygon
};

class FAIRYGUI_API SShape : public SDisplayObject, public FGCObject
{
public:
protected:
	EFGUIShapeType	  m_ShapeType = EFGUIShapeType::None;
	float			  m_LineWidth = 0;
	FColor			  m_LineColor = FColor::White;
	TOptional<FColor> m_CenterColor;
	TArray<FColor>	  m_Colors;
	float			  m_TopLeftRadius = 0;
	float			  m_TopRightRadius = 0;
	float			  m_BottomLeftRadius = 0;
	float			  m_BottomRightRadius = 0;
	float			  m_StartDegree = 0;
	float			  m_EndDegree = 360;
	TArray<FVector2D> m_PolygonPoints;
	bool			  m_bUsePercentPositions = false;
	int32			  m_PolygonSides = 3;
	float			  m_PolygonRotation = 0;
	TArray<float>	  m_PolygonDistances;

	mutable TObjectPtr<UNTexture> m_Texture;
	TArray<FVector2D>			  m_Texcoords;

	mutable TSharedPtr<FSubMesh> m_pSubMesh;

public:
	DECLARE_TYPE_ID(SShape, SDisplayObject)

	explicit SShape(UGObject* InGObject = nullptr);

	EFGUIShapeType GetType() const { return m_ShapeType; }
	void		   SetType(EFGUIShapeType InShapeType)
	{
		if (m_ShapeType != InShapeType)
		{
			m_ShapeType = InShapeType;
			m_bRebuildMesh = true;
		}
	}

	float GetLineWidth() const { return m_LineWidth; }
	void  SetLineWidth(float InLineWidth)
	{
		if (m_LineWidth != InLineWidth)
		{
			m_LineWidth = InLineWidth;
			m_bRebuildMesh = true;
		}
	}

	FColor GetLineColor() const { return m_LineColor; }
	void   SetLineColor(const FColor& InLineColor)
	{
		if (m_LineColor != InLineColor)
		{
			m_LineColor = InLineColor;
			m_bRebuildMesh = true;
		}
	}

	bool   HaveCenterColor() const { return m_CenterColor.IsSet(); }
	FColor GetCenterColor() const { return m_CenterColor.Get(FColor::Transparent); }
	void   SetCenterColor(const FColor* pCenterColor)
	{
		if (pCenterColor)
		{
			if (!m_CenterColor.IsSet() || m_CenterColor.GetValue() != *pCenterColor)
			{
				m_CenterColor = *pCenterColor;
				m_bRebuildMesh = true;
			}
		}
		else if (m_CenterColor.IsSet())
		{
			m_CenterColor.Reset();
			m_bRebuildMesh = true;
		}
	}

	const TArray<FColor>& GetColors() const { return m_Colors; }
	void				  SetColors(const TArray<FColor>& InColors)
	{
		m_Colors = InColors;
		m_bRebuildMesh = true;
	}

	float GetTopLeftRadius() const { return m_TopLeftRadius; }
	void  SetTopLeftRadius(float InRadius)
	{
		if (m_TopLeftRadius != InRadius)
		{
			m_TopLeftRadius = InRadius;
			m_bRebuildMesh = true;
		}
	}
	float GetTopRightRadius() const { return m_TopRightRadius; }
	void  SetTopRightRadius(float InRadius)
	{
		if (m_TopRightRadius != InRadius)
		{
			m_TopRightRadius = InRadius;
			m_bRebuildMesh = true;
		}
	}
	float GetBottomLeftRadius() const { return m_BottomLeftRadius; }
	void  SetBottomLeftRadius(float InRadius)
	{
		if (m_BottomLeftRadius != InRadius)
		{
			m_BottomLeftRadius = InRadius;
			m_bRebuildMesh = true;
		}
	}
	float GetBottomRightRadius() const { return m_BottomRightRadius; }
	void  SetBottomRightRadius(float InRadius)
	{
		if (m_BottomRightRadius != InRadius)
		{
			m_BottomRightRadius = InRadius;
			m_bRebuildMesh = true;
		}
	}
	float GetStartDegree() const { return m_StartDegree; }
	void  SetStartDegree(float InDegree)
	{
		if (m_StartDegree != InDegree)
		{
			m_StartDegree = InDegree;
			m_bRebuildMesh = true;
		}
	}
	float GetEndDegree() const { return m_EndDegree; }
	void  SetEndDegree(float InDegree)
	{
		if (m_EndDegree != InDegree)
		{
			m_EndDegree = InDegree;
			m_bRebuildMesh = true;
		}
	}

	const TArray<FVector2D>& GetPolygonPoints() const { return m_PolygonPoints; }
	void					 SetPolygonPoints(const TArray<FVector2D>& InPoints)
	{
		m_PolygonPoints = InPoints;
		m_bRebuildMesh = true;
	}

	bool GetUsePercentPositions() const { return m_bUsePercentPositions; }
	void SetUsePercentPositions(bool bInUsePercentPositions)
	{
		if (m_bUsePercentPositions != bInUsePercentPositions)
		{
			m_bUsePercentPositions = bInUsePercentPositions;
			m_bRebuildMesh = true;
		}
	}

	int32 GetPolygonSides() const { return m_PolygonSides; }
	void  SetPolygonSides(int32 InSides)
	{
		if (m_PolygonSides != InSides)
		{
			m_PolygonSides = InSides;
			m_bRebuildMesh = true;
		}
	}

	float GetPolygonRotation() const { return m_PolygonRotation; }
	void  SetPolygonRotation(float InRotation)
	{
		if (m_PolygonRotation != InRotation)
		{
			m_PolygonRotation = InRotation;
			m_bRebuildMesh = true;
		}
	}

	const TArray<float>& GetPolygonDistances() const { return m_PolygonDistances; }
	void				 SetPolygonDistances(const TArray<float>& InDistances)
	{
		m_PolygonDistances = InDistances;
		m_bRebuildMesh = true;
	}

	UNTexture* GetTexture() const { return m_Texture; }
	void	   SetTexture(UNTexture* InTexture)
	{
		if (m_Texture != InTexture)
		{
			m_Texture = InTexture;
			m_bRebuildMesh = true;
		}
	}

	const TArray<FVector2D>& GetTexcoords() const { return m_Texcoords; }
	void					 SetTexcoords(const TArray<FVector2D>& InTexcoords)
	{
		m_Texcoords = InTexcoords;
		m_bRebuildMesh = true;
	}

public:
	virtual void CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
		float InSaturation) const override;

	virtual const SDisplayObject* HitTest(const FVector2D& GlobalPoint) const override;

	virtual void	AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

protected:
	void RebuildMesh(int32 LayerID);
};