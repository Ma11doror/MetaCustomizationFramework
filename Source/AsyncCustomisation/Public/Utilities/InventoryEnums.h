#pragma once

#include "CoreMinimal.h"
#include "InventoryEnums.generated.h"

UENUM(BlueprintType)
enum class EInventorySection : uint8
{
    None UMETA(DisplayName = "None"),
    Equipment UMETA(DisplayName = "Equipment"),
    Customization UMETA(DisplayName = "Customization")
};

UENUM(BlueprintType)
enum class ESortingMethod : uint8
{
    None UMETA(DisplayName = "None"),
    ByName UMETA(DisplayName = "By Name"),
    ByTier UMETA(DisplayName = "By Tier"),
    ByDurability UMETA(DisplayName = "By Durability")
};

UENUM(BlueprintType)
enum class ESortingOrder : uint8
{
    Ascending UMETA(DisplayName = "Ascending"),
    Descending UMETA(DisplayName = "Descending")
}; 