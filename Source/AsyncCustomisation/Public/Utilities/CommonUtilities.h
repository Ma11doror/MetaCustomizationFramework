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

	inline FGameplayTag GetItemSlotTagForSlug(const FName& ItemSlug, TMap<FName, FGameplayTag>& InOutCache)
	{
		// 1. Check cache first
		if (const FGameplayTag* FoundTag = InOutCache.Find(ItemSlug))
		{
			return *FoundTag;
		}

		auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
		if (!AssetManager)
		{
			UE_LOG(LogTemp, Error, TEXT("GetItemSlotTagForSlug: AssetManager is null."));
			return FGameplayTag();
		}

		// 2. Find the META asset data in the registry
		FAssetData MetaAssetData;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		const TArray<FPrimaryAssetType> MetaAssetTypesToSearch = {
			GLOBAL_CONSTANTS::PrimaryItemAssetType,
			GLOBAL_CONSTANTS::PrimaryItemShaderAssetType
		};

		for (const FPrimaryAssetType& AssetType : MetaAssetTypesToSearch)
		{
			const FPrimaryAssetId PotentialMetaAssetId(AssetType, ItemSlug);
			const FSoftObjectPath AssetPathToFind = AssetManager->GetPrimaryAssetPath(PotentialMetaAssetId);
			if (AssetPathToFind.IsValid())
			{
				MetaAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(AssetPathToFind);
			}
			if (MetaAssetData.IsValid())
			{
				break;
			}
		}

		if (!MetaAssetData.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("GetItemSlotTagForSlug (Registry): FAILED to find META asset data for slug '%s'."), *ItemSlug.ToString());
			InOutCache.Add(ItemSlug, FGameplayTag());
			return FGameplayTag();
		}

		// 3. Get the CustomizationAssetId string from the asset data tags
		FString CustomizationAssetIdString;
		if (!MetaAssetData.GetTagValue("CustomizationAssetId", CustomizationAssetIdString) || CustomizationAssetIdString.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("GetItemSlotTagForSlug (Registry): Meta asset data for '%s' does not have a valid 'CustomizationAssetId' tag."), *ItemSlug.ToString());
			InOutCache.Add(ItemSlug, FGameplayTag());
			return FGameplayTag();
		}

		// 4. Convert the string back to FPrimaryAssetId by parsing it
		FPrimaryAssetId CustomizationAssetId;
		FString AssetTypeString, AssetNameString;
		if (CustomizationAssetIdString.Split(TEXT(":"), &AssetTypeString, &AssetNameString))
		{
			CustomizationAssetId = FPrimaryAssetId(FPrimaryAssetType(*AssetTypeString), FName(*AssetNameString));
		}

		if (!CustomizationAssetId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("GetItemSlotTagForSlug (Registry): Could not parse CustomizationAssetId from string '%s' for slug '%s'."), *CustomizationAssetIdString, *ItemSlug.ToString());
			InOutCache.Add(ItemSlug, FGameplayTag());
			return FGameplayTag();
		}

		// 5. Now, load the ACTUAL customization asset using its correct ID
		UPrimaryDataAsset* LoadedCustomizationAsset = AssetManager->LoadPrimaryAsset<UPrimaryDataAsset>(CustomizationAssetId);

		if (!LoadedCustomizationAsset)
		{
			UE_LOG(LogTemp, Error, TEXT("GetItemSlotTagForSlug (by loading): FAILED to load customization asset with ID '%s'. CHECK ASSET MANAGER SETTINGS!"), *CustomizationAssetId.ToString());
			InOutCache.Add(ItemSlug, FGameplayTag());
			return FGameplayTag();
		}

		// 6. Extract the slot tag from the loaded customization asset
		FGameplayTag ResultTag;
		if (auto* BodyPartAsset = Cast<UBodyPartAsset>(LoadedCustomizationAsset))
		{
			ResultTag = BodyPartAsset->TargetItemSlot;
		}
		else if (auto* MaterialAsset = Cast<UMaterialCustomizationDataAsset>(LoadedCustomizationAsset))
		{
			ResultTag = MaterialAsset->TargetItemSlot;
		}
		else if (auto* MaterialPack = Cast<UMaterialPackCustomizationDA>(LoadedCustomizationAsset))
		{
			if (!MaterialPack->MaterialAsset.MaterialCustomizations.IsEmpty() && MaterialPack->MaterialAsset.MaterialCustomizations[0])
			{
				ResultTag = MaterialPack->MaterialAsset.MaterialCustomizations[0]->TargetItemSlot;
			}
		}

		UE_LOG(LogTemp, Log, TEXT("GetItemSlotTagForSlug: Successfully got slot '%s' for slug '%s'."), *ResultTag.ToString(), *ItemSlug.ToString());

		// 7. Update cache and return
		InOutCache.Add(ItemSlug, ResultTag);
		return ResultTag;
	}
}
