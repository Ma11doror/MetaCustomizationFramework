#pragma once

#include "CoreMinimal.h"
#include "Components/Core/CustomizationSlotTypes.h"
#include "Engine/DataAsset.h"
#include "CustomizationSlotMappingAsset.generated.h"

UCLASS()
class ASYNCCUSTOMISATION_API UCustomizationSlotMappingAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The core mapping definition. Designers edit this map. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mapping", meta = (TitleProperty = "Value"))
	TMap<EItemSlot, ECustomizationSlotType> ItemTypeToSlotMap;

	/**
	 * Gets the customization slot type for a given item type based on the map.
	 * @param ItemType The item type to look up.
	 * @return The corresponding ECustomizationSlotType, or ECustomizationSlotType::Unknown if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "Mapping")
	ECustomizationSlotType GetSlotTypeForItem(EItemSlot ItemType) const
	{
		const ECustomizationSlotType* FoundSlot = ItemTypeToSlotMap.Find(ItemType);
		return FoundSlot ? *FoundSlot : ECustomizationSlotType::Unknown;
	}
};
