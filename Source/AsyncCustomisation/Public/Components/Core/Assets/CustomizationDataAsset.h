// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Constants/GlobalConstants.h"
#include "Engine/DataAsset.h"
#include "CustomizationDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FCustomizationComplect
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RelativeTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName SocketName;
};

UCLASS(HideCategories = "CustomizationCondition")
class ASYNCCUSTOMISATION_API UCustomizationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FCustomizationComplect> CustomizationComplect;

	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryCustomizationAssetType, GetFName());
	}

};
