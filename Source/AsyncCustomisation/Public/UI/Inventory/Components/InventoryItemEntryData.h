
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryItemEntryData.generated.h"


UCLASS()
class ASYNCCUSTOMISATION_API UInventoryItemEntryData : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	FPrimaryAssetId AssetId;
};
