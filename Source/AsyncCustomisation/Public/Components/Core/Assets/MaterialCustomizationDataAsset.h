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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MaterialCustomization",
		meta = (EditCondition = "bApplyOnBodyPart", EditConditionHides))
	EBodyPartType BodyPartType = EBodyPartType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MaterialCustomization",
		meta = (EditCondition = "!bApplyOnBodyPart", EditConditionHides))
	FName SocketName;

	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType, GetFName());
	}
};
