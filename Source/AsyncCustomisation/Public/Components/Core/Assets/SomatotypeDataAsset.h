// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SomatotypeDataAsset.generated.h"

struct FSkinFlagCombination;
class UBodyPartAsset;


UCLASS()
class ASYNCCUSTOMISATION_API USomatotypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TMap<FSkinFlagCombination, FBodySkinAsset> SkinAssociation;
	
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SomatotypeDataAsset", GetFName());
	}
};
