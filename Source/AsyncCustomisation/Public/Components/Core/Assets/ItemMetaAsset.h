#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "AsyncCustomisation/Public/Components/Core/Data.h"
#include "Engine/DataAsset.h"
#include "ItemMetaAsset.generated.h"

UCLASS()
class ASYNCCUSTOMISATION_API UItemMetaAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;

	//TODO:: deprecated? 
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon_Big;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (MultiLine = false))
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (MultiLine = true))
	FText Description;
	
	// UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Classification", meta=(AssetRegistrySearchable)) 
	// EItemSlot ItemSlot = EItemSlot::None;
	
	/** The UI category this item belongs to. e.g., 'Slot.UI.Back' */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Classification", meta=(AssetRegistrySearchable, GameplayTagFilter="Slot.UI")) 
	FGameplayTag UISlotCategoryTag;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Classification", meta=(AssetRegistrySearchable)) 
	EItemType ItemType = EItemType::Clothing;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Classification", meta=(AssetRegistrySearchable)) 
	EItemTier ItemTier = EItemTier::Common;

	// NOTE: ReadWrite only for Editor Utilities
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AssetRegistrySearchable))
	FPrimaryAssetId CustomizationAssetId;
	
	UPROPERTY(EditDefaultsOnly, Category = "Customization", meta = (EditCondition = "bSupportsSkins"))
	bool bSupportsSkins = false;

	/**
	* List of available skin meta-assets applicable to this item.
	*
	* Only meta-assets that internally reference a valid `MaterialCustomizationDataAsset` should be added to this list.
	* These assets must be derived from `UCustomizationDataAsset` and properly registered with the Asset Manager.
	*
	* Adding incompatible or unrelated asset types may result in skin functionality not working as expected.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Customization|Skins", meta = (EditCondition = "bSupportsSkins", EditConditionHides, AllowedClasses = "ItemMetaAsset"))  //, AllowedClasses = "MaterialCustomizationDataAsset")
	TArray<FPrimaryAssetId> AvailableSkinAssetIds;
	
#if WITH_EDITOR
	
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override
	{
		Super::GetAssetRegistryTags(OutTags);

		// Manually add tags for properties that need to be queried from the asset registry.
		OutTags.Add(FAssetRegistryTag("ItemType", UEnum::GetValueAsString(ItemType), FAssetRegistryTag::ETagType::TT_Alphabetical));
		OutTags.Add(FAssetRegistryTag("UISlotCategoryTag", UISlotCategoryTag.ToString(), FAssetRegistryTag::ETagType::TT_Alphabetical));
		OutTags.Add(FAssetRegistryTag("CustomizationAssetId", CustomizationAssetId.ToString(), FAssetRegistryTag::TT_Alphabetical));
	}
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		const FName PropertyName = PropertyChangedEvent.GetPropertyName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UItemMetaAsset, CustomizationAssetId))
		{
			const auto Type = CustomizationAssetId.PrimaryAssetType.GetName();
			bSupportsSkins = !Type.IsEqual(GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType) && !Type.IsEqual(GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType);
		}
	}
#endif
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryItemAssetType, GetFName());
	}
};

UCLASS()
class ASYNCCUSTOMISATION_API UItemShaderMetaAsset : public UItemMetaAsset
{
	GENERATED_BODY()

public:
	/**
	 * Tint color for item shader icon
	 */

	UItemShaderMetaAsset()
	{
		ItemType = EItemType::Skin;
	}
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Customization")
	FLinearColor ColorTint;

	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryItemShaderAssetType, GetFName());
	}
};
