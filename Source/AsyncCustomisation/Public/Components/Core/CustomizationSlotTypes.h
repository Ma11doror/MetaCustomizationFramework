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
		inline static const Util::TFixedMap<EItemSlot, ECustomizationSlotType, 32> CustomizationSlotsMapping
		{
			// mains
			{EItemSlot::Body, ECustomizationSlotType::Body},
			{EItemSlot::Head, ECustomizationSlotType::Head},
			{EItemSlot::Hat, ECustomizationSlotType::Hat},
			{EItemSlot::Feet, ECustomizationSlotType::Feet},
			{EItemSlot::Hands, ECustomizationSlotType::Hands},
			{EItemSlot::Wrists, ECustomizationSlotType::Wrists}, 
			{EItemSlot::Legs, ECustomizationSlotType::Legs},

			// extern items
			{EItemSlot::Beard, ECustomizationSlotType::Beard}, 
			{EItemSlot::Haircut, ECustomizationSlotType::Haircut},
			{EItemSlot::Skin, ECustomizationSlotType::Skin},

			// Accessory
			{EItemSlot::Ring, ECustomizationSlotType::Ring},
			{EItemSlot::Chain, ECustomizationSlotType::Pendant}, 
			{EItemSlot::Bandan, ECustomizationSlotType::FaceAccessory}, //  Pendant?
			{EItemSlot::FaceAccessory, ECustomizationSlotType::FaceAccessory}, 

			{EItemSlot::Backpack, ECustomizationSlotType::Back},
			{EItemSlot::Cloak, ECustomizationSlotType::Back},
		};
	}

	inline ECustomizationSlotType GetSlotForItem(EItemSlot ItemSlot)
	{
		const ECustomizationSlotType* Found = Detail::CustomizationSlotsMapping.Find(ItemSlot);
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
		
		const FName ItemSlotTagName = GET_MEMBER_NAME_CHECKED(UItemMetaAsset, ItemSlot);
		EItemSlot ItemSlotValue = GetEnumValueFromAssetData<EItemSlot>(
			AssetData,
			ItemSlotTagName,
			EItemSlot::None 
		);
		
		return GetSlotForItem(ItemSlotValue);
	}
	inline EItemSlot GetItemSlotForSlug(FName ItemSlug)
	{
		return  EItemSlot::None;
	};
} // namespace CustomizationSlots
