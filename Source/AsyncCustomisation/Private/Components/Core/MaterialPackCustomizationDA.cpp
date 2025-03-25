
#include "AsyncCustomisation/Public/Components/Core/Assets/MaterialPackCustomizationDA.h"

#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"

FPrimaryAssetId UMaterialPackCustomizationDA::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType, GetFName());
}
