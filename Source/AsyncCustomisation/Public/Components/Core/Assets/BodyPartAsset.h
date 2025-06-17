#pragma once

#include "CoreMinimal.h"

#include "Algo/AllOf.h"
#include "Constants/GlobalConstants.h"
#include "Engine/DataAsset.h"
#include "Misc/RuntimeErrors.h"
#include "AsyncCustomisation/Public/Components/Core/Data.h"
#include "AsyncCustomisation/Public/Components/Core/BodyPartTypes.h"
#include "BodyPartAsset.generated.h"

enum class EBodyPartType : uint8;

USTRUCT(BlueprintType)
struct FBodyPartVariant
{
	GENERATED_USTRUCT_BODY()

	// Body part
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USkeletalMesh* BodyPartSkeletalMesh = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TObjectPtr<UMaterialInterface>> DefaultMaterials;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EBodyPartType BodyPartType = EBodyPartType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Requirements")
	TArray<FPrimaryAssetId> RequiredItemsAssetIds;

	//Mark how this asset affect to skin visibility
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Requirements")
	FSkinFlagCombination SkinCoverageFlags;

	bool IsValid() const { return BodyPartSkeletalMesh && BodyPartType != EBodyPartType::None; }
};

USTRUCT(BlueprintType)
struct FBodySkinAsset
{
	GENERATED_USTRUCT_BODY()
	
	// Skin skeletal mesh
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USkeletalMesh* BodyPartSkeletalMesh = nullptr;
};

UCLASS()
class ASYNCCUSTOMISATION_API UBodyPartAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FBodyPartVariant> Variants;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryBodyPartAssetType, GetFName());
	}

	const FBodyPartVariant* GetMatchedVariant(TArray<FPrimaryAssetId> EquippedItemAssetIds) const
	{
		auto* MatchedVariantPtr = Variants.FindByPredicate([EquippedItemAssetIds](const FBodyPartVariant& InVariant)
		{
			return !InVariant.RequiredItemsAssetIds.IsEmpty()
				&& Algo::AllOf(InVariant.RequiredItemsAssetIds,
				               [EquippedItemAssetIds](const FPrimaryAssetId& InRequiredAssetId)
				               {
					               return EquippedItemAssetIds.Contains(InRequiredAssetId);
				               });
		});
		// Try find matched variant first.
		if (MatchedVariantPtr)
		{
			if (!ensureAsRuntimeWarning(MatchedVariantPtr->IsValid()))
			{
				return nullptr;
			}
			return MatchedVariantPtr;
		}

		// Otherwise try return with no requirements, if exist
		auto* DefaultVariantPtr =
			Variants.FindByPredicate([](const FBodyPartVariant& InVariant)
			{
				return InVariant.RequiredItemsAssetIds.IsEmpty();
			});
		if (!ensureAsRuntimeWarning(DefaultVariantPtr && DefaultVariantPtr->IsValid()))
		{
			return nullptr;
		}
		return DefaultVariantPtr;
	}
};
