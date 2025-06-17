#pragma once

#include "CoreMinimal.h"

namespace GLOBAL_CONSTANTS
{
	inline const FSoftObjectPath DATA_TABLE_LIBRARY = FSoftObjectPath(TEXT("/Game/Data/DT_DataTableLibrary.DT_DataTableLibrary"));
	inline const FString NONE_STRING = TEXT("NONE");
	inline const FName NONE_FNAME = TEXT("NONE");

	inline const FPrimaryAssetId NONE_ASSET_ID = FPrimaryAssetId();
	
	inline const FName PrimaryItemAssetType = FName(TEXT("ItemMetaAsset"));
	inline const FName PrimaryItemShaderAssetType = FName(TEXT("ItemShaderMetaAsset"));
	inline const FName PrimarySomatotypeAssetType = FName(TEXT("SomatotypeDataAsset"));

	inline const FName PrimaryCustomizationAssetType = FName(TEXT("CustomizationDataAsset")); // TODO:: rename!!!
	inline const FName PrimaryMaterialCustomizationAssetType = FName(TEXT("MaterialCustomizationDataAsset"));
	inline const FName PrimaryMaterialPackCustomizationAssetType = FName(TEXT("MaterialPackCustomizationDA"));
	inline const FName PrimaryBodyPartAssetType = FName(TEXT("BodyPartAsset"));
	
	inline const FName CustomizationTag = FName(TEXT("Customization"));
	
} // namespace GLOBAL_CONSTANTS