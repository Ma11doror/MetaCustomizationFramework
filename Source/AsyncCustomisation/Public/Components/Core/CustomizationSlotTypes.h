#pragma once

#include "InlineContainers.h"
#include "Data.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Assets/ItemMetaAsset.h"


// TODO should be placed it inside DataAsset and allow option to configure slots for game designers
UENUM()
enum class ECustomizationSlotType : uint8
{
	Unknown = 0,
	Body, // ( >1, if OnlyOneItemInSlot=false)
	Hat,
	Head,
	Feet,
	Hands,
	Wrists,
	Legs,
	Beard,
	Ring,
	Skin,
	Haircut,
	Pendant,
	// NOTE: Types bellow are just to be sure we have enough slots
	FaceAccessory,
	Back,
};
ENUM_RANGE_BY_FIRST_AND_LAST(ECustomizationSlotType, ECustomizationSlotType::Unknown, ECustomizationSlotType::Back);

namespace CustomizationSlots
{
	namespace Detail
	{
		inline static const Util::TFixedMap<EItemType, ECustomizationSlotType, 32> CustomizationSlotsMapping
		{
			// mains
			{EItemType::Body, ECustomizationSlotType::Body},
			{EItemType::Head, ECustomizationSlotType::Head},
			{EItemType::Hat, ECustomizationSlotType::Hat},
			{EItemType::Feet, ECustomizationSlotType::Feet},
			{EItemType::Hands, ECustomizationSlotType::Hands},
			{EItemType::Wrists, ECustomizationSlotType::Wrists}, 
			{EItemType::Legs, ECustomizationSlotType::Legs},

			// extern items
			{EItemType::Beard, ECustomizationSlotType::Beard}, 
			{EItemType::Haircut, ECustomizationSlotType::Haircut},
			{EItemType::Skin, ECustomizationSlotType::Skin},

			// Accessory
			{EItemType::Ring, ECustomizationSlotType::Ring},
			{EItemType::Chain, ECustomizationSlotType::Pendant}, 
			{EItemType::Bandan, ECustomizationSlotType::FaceAccessory}, //  Pendant?
			{EItemType::FaceAccessory, ECustomizationSlotType::FaceAccessory}, 

			{EItemType::Backpack, ECustomizationSlotType::Back},
			{EItemType::Cloak, ECustomizationSlotType::Back},
		};
	}

	inline ECustomizationSlotType GetSlotTypeForItem(EItemType ItemType)
	{
		const ECustomizationSlotType* Found = Detail::CustomizationSlotsMapping.Find(ItemType);
		return Found ? *Found : ECustomizationSlotType::Unknown;
	}

	template <typename EnumType>
		EnumType GetEnumValueFromAssetData(const FAssetData& AssetData, const FName TagName, EnumType DefaultValue)
	{
		FString TagValue;
		if (AssetData.GetTagValue(TagName, TagValue))
		{
			const UEnum* EnumPtr = StaticEnum<EnumType>();
			if (EnumPtr)
			{
				int64 IntValue = EnumPtr->GetValueByNameString(TagValue);
				if (IntValue != INDEX_NONE)
				{
					return static_cast<EnumType>(IntValue);
				}
			}
		}
		return DefaultValue;
	}
	
	inline ECustomizationSlotType GetSlotTypeForItemBySlug(const UObject* WCO, FName ItemSlug)
	{
		
		if (ItemSlug == NAME_None)
		{
			return ECustomizationSlotType::Unknown;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		const FPrimaryAssetId MetaAssetId(GLOBAL_CONSTANTS::PrimaryItemAssetType, ItemSlug);
		if (!MetaAssetId.IsValid())
		{
			return ECustomizationSlotType::Unknown;
		}

		FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(MetaAssetId.ToString()));

		if (!AssetData.IsValid())
		{
			return ECustomizationSlotType::Unknown;
		}
		
		const FName ItemTypeTagName = GET_MEMBER_NAME_CHECKED(UItemMetaAsset, ItemType);
		EItemType ItemTypeValue = GetEnumValueFromAssetData<EItemType>(
			AssetData,
			ItemTypeTagName,
			EItemType::None 
		);
		
		return GetSlotTypeForItem(ItemTypeValue);
	}
	inline EItemType GetItemTypeForSlug(FName ItemSlug)
	{
		return  EItemType::None;
	};
} // namespace CustomizationSlots
