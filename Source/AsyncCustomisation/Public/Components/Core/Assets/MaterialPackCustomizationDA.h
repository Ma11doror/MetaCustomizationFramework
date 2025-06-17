#pragma once

#include "MaterialPackCustomizationDA.generated.h"

class UMaterialCustomizationDataAsset;

USTRUCT(BlueprintType)
struct FMaterialCustomizationCollection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<UMaterialCustomizationDataAsset*> MaterialCustomizations;
};

UCLASS()
class ASYNCCUSTOMISATION_API UMaterialPackCustomizationDA : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FMaterialCustomizationCollection MaterialAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UMaterialInterface> CustomMaterial = nullptr;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType, GetFName());
	}
};
