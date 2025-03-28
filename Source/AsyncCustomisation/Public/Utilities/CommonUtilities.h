#pragma once

#include <CoreMinimal.h>

#include "CustomizationAssetManager.h"
#include "AsyncCustomisation/Public/Components/Core/Assets/ItemMetaAsset.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"

using namespace GLOBAL_CONSTANTS;


namespace CommonUtilities
{
	inline FPrimaryAssetId ItemSlugToCustomizationAssetId(const FName& InSlug, const ESomatotype InSomatotype = ESomatotype::None)
	{
		FPrimaryAssetId ItemCustomizationAssetId = NONE_ASSET_ID;
		const UItemMetaAsset* ItemMetaAsset = UCustomizationAssetManager::LoadItemMetaAssetSync(InSlug);
		
		if (ItemMetaAsset && ItemMetaAsset->CustomizationAssetId != NONE_ASSET_ID)
		{
			ItemCustomizationAssetId = ItemMetaAsset->CustomizationAssetId;
		}

		//TODO:: deprecated
		//if (ItemMetaAsset && InSomatotype != ESomatotype::None)
		//{
		//	if (const FPrimaryAssetId* FoundCustomizationItem = ItemMetaAsset->BySomatotypeBodyPartAssetId.Find(InSomatotype))
		//	{
		//		ItemCustomizationAssetId = *FoundCustomizationItem;
		//	}
		//}
		return ItemCustomizationAssetId;
	}
	
	inline FPrimaryAssetId ItemSlugToAssetId(const FName& InSlug)
	{
		FPrimaryAssetId AssetId = {};
		UE_LOG(LogTemp, Warning, TEXT("Attempting to create AssetId with slug: %s (Length: %d)"), 
	   *InSlug.ToString(), InSlug.ToString().Len());

		if (!InSlug.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("ItemSlugToAssetId:: NOT VALID InSlug"));
		}
		if (InSlug.ToString().Len() > NAME_SIZE)
		{
			UE_LOG(LogTemp, Warning, TEXT("ItemSlugToAssetId:: <= NAME_SIZE :: %s"), *InSlug.ToString());
		}
		
		AssetId.PrimaryAssetName = InSlug;
		UE_LOG(LogTemp, Warning, TEXT("AssetId.PrimaryAssetName:: %s"), *AssetId.PrimaryAssetName.ToString());
		
		AssetId.PrimaryAssetType = GLOBAL_CONSTANTS::PrimaryItemAssetType;
		//UE_LOG(LogTemp, Warning, TEXT("AssetId.PrimaryAssetType:: %s"), *AssetId.PrimaryAssetType.ToString());
		return AssetId;
	}

	inline TArray<FPrimaryAssetId> ItemSlugsToAssetIds(const TArray<FName>& InSlugs)
	{
		TArray<FPrimaryAssetId> Result;
		for (const FName& Slug : InSlugs)
		{
			const FPrimaryAssetId AssetId = ItemSlugToAssetId(Slug);
			Result.Add(AssetId);
		}
		return Result;
	}

}