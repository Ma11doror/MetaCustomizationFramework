#pragma once

#include "CoreMinimal.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"

#include "Engine/DataAsset.h"
#include "ItemMetaAsset.generated.h"

UCLASS()
class ASYNCCUSTOMISATION_API UItemMetaAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon_Big;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (MultiLine = true))
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (MultiLine = true))
	FText Description;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Classification", meta=(AssetRegistrySearchable)) 
	EItemType ItemType = EItemType::None;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Classification", meta=(AssetRegistrySearchable)) 
	EItemTier ItemTier = EItemTier::Common;

	// NOTE: ReadWrite only for Editor Utilities
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable)
	FPrimaryAssetId CustomizationAssetId;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryItemAssetType, GetFName());
	}
};