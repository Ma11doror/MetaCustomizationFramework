#pragma once

#include "InlineContainers.h"
#include "Data.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Assets/ItemMetaAsset.h"
#include "Utilities/CustomizationAssetManager.h"


// TODO should be placed it inside DataAsset and allow option to configure slots for game designers
// UENUM()
// enum class ECustomizationSlotType : uint8
// {
// 	Unknown = 0,
// 	Body, // ( >1, if OnlyOneItemInSlot=false)
// 	Hat,
// 	Head,
// 	Feet,
// 	Hands,
// 	Wrists,
// 	Legs,
// 	Beard,
// 	Ring,
// 	Skin,
// 	Haircut,
// 	Pendant,
// 	// NOTE: Types bellow are just to be sure we have enough slots
// 	FaceAccessory,
// 	Back,
// };
// ENUM_RANGE_BY_FIRST_AND_LAST(ECustomizationSlotType, ECustomizationSlotType::Unknown, ECustomizationSlotType::Back);

namespace CustomizationSlots
{
	namespace Detail
	{
		// 	inline static const Util::TFixedMap<EItemSlot, ECustomizationSlotType, 32> CustomizationSlotsMapping
		// 	{
		// 		// mains
		// 		// {EItemSlot::Body, ECustomizationSlotType::Body},
		// 		// {EItemSlot::Head, ECustomizationSlotType::Head},
		// 		// {EItemSlot::Hat, ECustomizationSlotType::Hat},
		// 		// {EItemSlot::Feet, ECustomizationSlotType::Feet},
		// 		// {EItemSlot::Hands, ECustomizationSlotType::Hands},
		// 		// {EItemSlot::Wrists, ECustomizationSlotType::Wrists}, 
		// 		// {EItemSlot::Legs, ECustomizationSlotType::Legs},
		// 		//
		// 		// // extern items
		// 		// {EItemSlot::Beard, ECustomizationSlotType::Beard}, 
		// 		// {EItemSlot::Haircut, ECustomizationSlotType::Haircut},
		// 		// {EItemSlot::Skin, ECustomizationSlotType::Skin},
		// 		//
		// 		// // Accessory
		// 		// {EItemSlot::Ring, ECustomizationSlotType::Ring},
		// 		// {EItemSlot::Chain, ECustomizationSlotType::Pendant}, 
		// 		// {EItemSlot::Bandan, ECustomizationSlotType::FaceAccessory}, //  Pendant?
		// 		// {EItemSlot::FaceAccessory, ECustomizationSlotType::FaceAccessory}, 
		// 		//
		// 		// {EItemSlot::Backpack, ECustomizationSlotType::Back},
		// 		// {EItemSlot::Cloak, ECustomizationSlotType::Back},
		// 	};
	}

	// inline ECustomizationSlotType GetSlotForItem(EItemSlot ItemSlot)
	// {
	// 	const ECustomizationSlotType* Found = Detail::CustomizationSlotsMapping.Find(ItemSlot);
	// 	return Found ? *Found : ECustomizationSlotType::Unknown;
	// }
	
	template <typename EnumType>
	EnumType GetEnumValueFromAssetData(const FAssetData& AssetData, const FName& PropertyName, EnumType DefaultValue)
	{
		FString TagValue;
		if (AssetData.GetTagValue(PropertyName, TagValue))
		{
			const UEnum* EnumPtr = StaticEnum<EnumType>();
			if (EnumPtr)
			{
				FString EnumValueString;
				TagValue.Split(TEXT("::"), nullptr, &EnumValueString, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				if (EnumValueString.IsEmpty())
				{
					EnumValueString = TagValue;
				}

				const int64 IntValue = EnumPtr->GetValueByNameString(EnumValueString);
				if (IntValue != INDEX_NONE)
				{
					return static_cast<EnumType>(IntValue);
				}
			}
		}
		return DefaultValue;
	}
	
	inline FGameplayTag GetGameplayTagFromAssetData(const FAssetData& AssetData, const FName& PropertyName)
	{
		FString TagString;
		if (AssetData.GetTagValue(PropertyName, TagString) && !TagString.IsEmpty())
		{
			// The tag is often stored in the format (TagName="Slot.UI.Hat")
			// We need to extract the actual tag name.
			TagString.RemoveFromStart(TEXT("(TagName=\""));
			TagString.RemoveFromEnd(TEXT("\")"));
			return FGameplayTag::RequestGameplayTag(FName(*TagString), false); // bErrorIfNotFound = false
		}
		return FGameplayTag();
	}
	
	struct FItemRegistryData
	{
		EItemType ItemType = EItemType::None;
		FGameplayTag InUISlotCategoryTag;
		bool IsValid() const { return ItemType != EItemType::None && InUISlotCategoryTag.IsValid(); }
	};
	
	inline FItemRegistryData GetItemDataFromRegistry(const FName& ItemSlug)
	{
		FItemRegistryData ResultData;
		if (ItemSlug.IsNone())
		{
			return ResultData;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
		if(!AssetManager)
		{
			UE_LOG(LogTemp, Error, TEXT("GetItemDataFromRegistry: Could not get CustomizationAssetManager."));
			return ResultData;
		}

		FAssetData AssetData;
		const TArray<FPrimaryAssetType> MetaAssetTypesToCheck = {
			GLOBAL_CONSTANTS::PrimaryItemAssetType,
			GLOBAL_CONSTANTS::PrimaryItemShaderAssetType
		};

		for (const FPrimaryAssetType& AssetType : MetaAssetTypesToCheck)
		{
			const FPrimaryAssetId PotentialAssetId(AssetType, ItemSlug);
			const FSoftObjectPath AssetPathToFind = AssetManager->GetPrimaryAssetPath(PotentialAssetId);
			if (AssetPathToFind.IsValid())
			{
				AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(AssetPathToFind);
			}
			if (AssetData.IsValid())
			{
				break;
			}
		}

		if (!AssetData.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("GetItemDataFromRegistry: Could not find AssetData for slug %s with any known meta type."), *ItemSlug.ToString());
			return ResultData;
		}
		
		UE_LOG(LogTemp, Warning, TEXT("==================== TAGS FOR META ASSET: %s ===================="), *ItemSlug.ToString());
		AssetData.PrintAssetData();
		UE_LOG(LogTemp, Warning, TEXT("================================================================="));

		ResultData.ItemType = GetEnumValueFromAssetData(AssetData, "ItemType", EItemType::None);
		ResultData.InUISlotCategoryTag = GetGameplayTagFromAssetData(AssetData, "UISlotCategoryTag");

		return ResultData;
	}

	/**
	 * [DEBUG/WORKAROUND] Fetches item data by SYNCHRONOUSLY loading the meta asset.
	 * Slower than using the registry, but more reliable for debugging registry issues.
	 */
	inline FItemRegistryData GetItemDataByLoading(const FName& ItemSlug)
	{
		FItemRegistryData ResultData;
		auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
		if (!AssetManager) return ResultData;

		// We have to try loading with known types, since we don't know which one it is.
		const TArray<FPrimaryAssetType> MetaAssetTypesToCheck = {
			GLOBAL_CONSTANTS::PrimaryItemAssetType,
			GLOBAL_CONSTANTS::PrimaryItemShaderAssetType
		};
		
		UItemMetaAsset* LoadedMetaAsset = nullptr;
		for (const FPrimaryAssetType& AssetType : MetaAssetTypesToCheck)
		{
			const FPrimaryAssetId PotentialAssetId(AssetType, ItemSlug);
			// Using the synchronous load function from the asset manager
			LoadedMetaAsset = AssetManager->LoadPrimaryAsset<UItemMetaAsset>(PotentialAssetId);;
			if (LoadedMetaAsset)
			{
				break;
			}
		}

		if (LoadedMetaAsset)
		{
			ResultData.ItemType = LoadedMetaAsset->ItemType;
			ResultData.InUISlotCategoryTag = LoadedMetaAsset->UISlotCategoryTag;
			UE_LOG(LogTemp, Warning, TEXT("GetItemDataByLoading: SUCCESSFULLY loaded asset for slug '%s'. Type: %s, Slot: %s"), 
				*ItemSlug.ToString(), *UEnum::GetValueAsString(ResultData.ItemType), *ResultData.InUISlotCategoryTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GetItemDataByLoading: FAILED to load any meta asset for slug '%s'"), *ItemSlug.ToString());
		}
		
		return ResultData;
	}
} // namespace CustomizationSlots
