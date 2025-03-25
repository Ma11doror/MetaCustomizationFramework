#pragma once

#include "InlineContainers.h"
#include "Data.h"
#include "MetaConfig.h"
#include "MetaConfigItemTypes.h"


// TODO should be placed it inside DataAsset and allow option to configure slots for game designers
UENUM()
enum class ECustomizationSlotType : uint8
{
	Unknown = 0,
	Body,
	Head,
	Feet,
	Hands,
	Ring,
	Skin,
	Haircut,

	// NOTE: Types bellow are just to be sure we have enough slots
	Pendant,
	Weighting,
};
ENUM_RANGE_BY_FIRST_AND_LAST(ECustomizationSlotType, ECustomizationSlotType::Unknown, ECustomizationSlotType::Weighting);

namespace CustomizationSlots
{
	namespace Detail
	{
		inline static const Util::TFixedMap<EItemType, ECustomizationSlotType, 16> CustomizationSlotsMapping {
			{ EItemType::Body, ECustomizationSlotType::Body },
			{ EItemType::Ring, ECustomizationSlotType::Ring },
			{ EItemType::Bandan, ECustomizationSlotType::Pendant },
			{ EItemType::Chain, ECustomizationSlotType::Pendant },
			{ EItemType::Haircut, ECustomizationSlotType::Haircut }, 
			{ EItemType::Weighting, ECustomizationSlotType::Weighting },
			{ EItemType::Skin, ECustomizationSlotType::Skin }
		};
	}

	inline ECustomizationSlotType GetSlotTypeForItem(EItemType ItemType)
	{
		const ECustomizationSlotType* Found = Detail::CustomizationSlotsMapping.Find(ItemType);
		return Found ? *Found : ECustomizationSlotType::Unknown;
	}


	inline ECustomizationSlotType GetSlotTypeForItemBySlug(const UObject* WCO, FName ItemSlug)
	{
		if (UMetaConfig::Get(WCO) == nullptr)
		{
			return ECustomizationSlotType::Unknown;
		}

		const auto* MetaConfig = UMetaConfig::GetItemConfigBySlug(WCO, ItemSlug);
		ensure(MetaConfig);
		//TODO:: log
		const auto Result = GetSlotTypeForItem(MetaConfig->Type);
		return Result;
	}
} // namespace CustomizationSlots
