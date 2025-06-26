// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SlotMappingAsset.generated.h"

struct FGameplayTagContainer;
struct FGameplayTag;
/**
 * 
 */
UCLASS()
class ASYNCCUSTOMISATION_API USlotMappingAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Maps a UI category tag (e.g., Slot.UI.Torso) to one or more technical item slots (e.g., Slot.Item.Torso, Slot.Item.Neck). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot Mapping", meta=(ForceInlineRow, ShowOnlyInnerProperties))
	TMap<FGameplayTag, FGameplayTagContainer> UISlotToTechnicalSlots;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{

		return FPrimaryAssetId(FPrimaryAssetType("SlotMappingAsset"), GetFName());
	}
};
