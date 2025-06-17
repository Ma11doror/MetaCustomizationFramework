// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BodyPartAsset.h"
#include "SomatotypeDataAsset.generated.h"

struct FSkinFlagCombination;

UCLASS(Blueprintable)
class ASYNCCUSTOMISATION_API USomatotypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TMap<FSkinFlagCombination, FBodySkinAsset> SkinAssociation;
	
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimarySomatotypeAssetType, GetFName());
		//return FPrimaryAssetId(FPrimaryAssetType("SomatotypeDataAsset"), GetFName());
	}
};
