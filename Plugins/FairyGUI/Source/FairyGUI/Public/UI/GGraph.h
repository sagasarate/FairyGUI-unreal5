#pragma once

#include "GObject.h"
#include "Widgets/SShape.h"
#include "GGraph.generated.h"

UCLASS(BlueprintType)
class FAIRYGUI_API UGGraph : public UGObject
{
	GENERATED_BODY()

public:
	UGGraph();
	virtual ~UGGraph();
	virtual void Dispose() override;

	virtual const FColor& GetColor() const override;
	virtual void		  SetColor(const FColor& InColor) override;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI", meta = (AutoCreateRefTerm = "LineColor,FillColor"))
	void DrawRect(float LineWidth, const FColor& LineColor, const FColor& FillColor);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI", meta = (AutoCreateRefTerm = "LineColor,FillColor"))
	void DrawRoundRect(float LineWidth, const FColor& LineColor, const FColor& FillColor, float TopLeftRadius,
		float TopRightRadius, float BottomLeftRadius, float BottomRightRadius);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI", meta = (AutoCreateRefTerm = "LineColor,FillColor"))
	void DrawEllipse(float LineWidth, const FColor& LineColor, const FColor& FillColor, float StartDegree = 0,
		float EndDegree = 360);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI", meta = (AutoCreateRefTerm = "LineColor,FillColor,Points"))
	void DrawPolygon(
		float LineWidth, const FColor& LineColor, const FColor& FillColor, const TArray<FVector2D>& Points);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI", meta = (AutoCreateRefTerm = "LineColor,FillColor,Distances"))
	void DrawRegularPolygon(int32 Sides, float LineWidth, const FColor& LineColor, const FColor& FillColor,
		float Rotation, const TArray<float>& Distances);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void Clear();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool IsEmpty() const;

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	EFGUIShapeType GetType() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetType(EFGUIShapeType InShapeType);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetLineWidth() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetLineWidth(float InLineWidth);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	FColor GetLineColor() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetLineColor(const FColor& InLineColor);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool HaveCenterColor() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	FColor GetCenterColor() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetCenterColor(const FColor& CenterColor);
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void ClearCenterColor();

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	const TArray<FColor>& GetColors() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetColors(const TArray<FColor>& InColors);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetTopLeftRadius() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetTopLeftRadius(float InRadius);
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetTopRightRadius() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetTopRightRadius(float InRadius);
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetBottomLeftRadius() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetBottomLeftRadius(float InRadius);
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetBottomRightRadius() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetBottomRightRadius(float InRadius);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetStartDegree() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetStartDegree(float InDegree);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetEndDegree() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetEndDegree(float InDegree);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	const TArray<FVector2D>& GetPolygonPoints() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetPolygonPoints(const TArray<FVector2D>& InPoints);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	bool GetUsePercentPositions() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetUsePercentPositions(bool bInUsePercentPositions);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	int32 GetPolygonSides() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetPolygonSides(int32 InSides);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	float GetPolygonRotation() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetPolygonRotation(float InRotation);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	const TArray<float>& GetPolygonDistances() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetPolygonDistances(const TArray<float>& InDistances);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	UNTexture* GetTexture() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetTexture(UNTexture* InTexture);

	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	const TArray<FVector2D>& GetTexcoords() const;
	UFUNCTION(BlueprintCallable, Category = "FairyGUI")
	void SetTexcoords(const TArray<FVector2D>& InTexcoords);

	virtual FNVariant GetProp(EObjectPropID PropID) const override;
	virtual void	  SetProp(EObjectPropID PropID, const FNVariant& InValue) override;

protected:
	virtual void CreateDisplayObject() override;
	virtual void SetupBeforeAdd(FByteBuffer* Buffer, int32 BeginPos) override;

private:
	TSharedPtr<SShape> Content;
};