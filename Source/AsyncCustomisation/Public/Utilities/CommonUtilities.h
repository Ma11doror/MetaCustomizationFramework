#pragma once

#include <CoreMinimal.h>

#include "CustomizationAssetManager.h"
#include "AsyncCustomisation/Public/Components/Core/Assets/ItemMetaAsset.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"

using namespace GLOBAL_CONSTANTS;


namespace CommonUtilities
{
	inline FPrimaryAssetId ItemSlugToCustomizationAssetId(const FName& InSlug)
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		UObject* FoundObject = nullptr;

		// 1. Find or load asset if it is base item type
		FPrimaryAssetId BaseItemAssetId(GLOBAL_CONSTANTS::PrimaryItemAssetType, InSlug);
		if (BaseItemAssetId.IsValid())
		{
			FoundObject = AssetManager.GetPrimaryAssetObject(BaseItemAssetId);
			if (!FoundObject)
			{
				// Loading sync for now
				FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(BaseItemAssetId);
				if (AssetPath.IsValid())
				{
					FoundObject = AssetPath.TryLoad();
				}
			}
		}

		// 2. If no result - try to find as shader item asset
		if (!FoundObject)
		{
			FPrimaryAssetId SkinAssetId(GLOBAL_CONSTANTS::PrimaryItemShaderAssetType, InSlug);
			if (SkinAssetId.IsValid())
			{
				FoundObject = AssetManager.GetPrimaryAssetObject(SkinAssetId);
				if (!FoundObject)
				{
					FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(SkinAssetId);
					if (AssetPath.IsValid())
					{
						FoundObject = AssetPath.TryLoad();
					}
				}
			}
		}

		// 3. 
		if (FoundObject)
		{
			if (const UItemMetaAsset* ItemMetaAsset = Cast<UItemMetaAsset>(FoundObject))
			{
				if (ItemMetaAsset->CustomizationAssetId.IsValid())
				{
					return ItemMetaAsset->CustomizationAssetId;
				}
			}
		}

		// 4. null
		return FPrimaryAssetId();
	}
	
	inline FPrimaryAssetId ItemSlugToAssetId(const FName& InSlug)
	{
		FPrimaryAssetId AssetId = {};
		UE_LOG(LogTemp, Warning, TEXT("Attempting to create AssetId with slug: %s (Length: %d)"), *InSlug.ToString(), InSlug.ToString().Len());

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