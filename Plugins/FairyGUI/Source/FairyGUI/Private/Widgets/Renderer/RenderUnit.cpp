#include "Widgets/Renderer/RenderUnit.h"
#include "UI/UIConfig.h"
#include "FairyCommons.h"

CIDStorage<FRenderUnit> FRenderUnit::Pool;

FRenderUnit* FRenderUnit::Borrow()
{
	if (Pool.GetBufferSize() <= 0)
		Pool.Create(256, 256, FUIConfig::Config.RenderUnitPoolGrowLimit);
	FRenderUnit* pInfo = Pool.NewObject();
	if (pInfo == nullptr)
	{
		UE_LOG(
			LogFairyGUI, Error, TEXT("FRenderUnit pool is full(%u/%u)!"), Pool.GetObjectCount(), Pool.GetBufferSize());
	}
	return pInfo;
}
void FRenderUnit::Return(FRenderUnit* pValue)
{
	uint32 ID = pValue->PoolID;
	pValue->Clear();
	Pool.DeleteObject(ID);
}