// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "Engine/DataAsset.h"

#include "AsyncCustomisation/Public/Components/Core/BodyPartTypes.h"

#include "MaterialCustomizationDataAsset.generated.h"

//enum class EBodyPartType : uint8;


UCLASS()
class ASYNCCUSTOMISATION_API UMaterialCustomizationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MaterialCustomization")
	TMap<int32, UMaterialInterface*> IndexWithApplyingMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MaterialCustomization")
	bool bApplyOnBodyPart = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MaterialCustomization", meta = (AssetRegistrySearchable, EditCondition = "bApplyOnBodyPart", EditConditionHides, GameplayTagFilter="Slot.Item"))
	FGameplayTag TargetItemSlot;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MaterialCustomization",
		meta = (EditCondition = "!bApplyOnBodyPart", EditConditionHides))
	FName SocketName;

#if WITH_EDITOR
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override
	{
		Super::GetAssetRegistryTags(OutTags);
		OutTags.Add(FAssetRegistryTag("TargetItemSlot", TargetItemSlot.ToString(), FAssetRegistryTag::ETagType::TT_Alphabetical));
	}
#endif
	
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType, GetFName());
	}
};
