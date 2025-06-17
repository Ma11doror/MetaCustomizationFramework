#pragma once

#include "CoreMinimal.h"
#include "AsyncCustomisation/Public/Utilities/CounterComponent.h"
#include "AsyncCustomisation/Public/Utilities/TimerComponent.h"
#include "Core/CharacterComponentBase.h"
#include "Core/CustomizationTypes.h"

#include "Core/BodyPartTypes.h"

#include "CustomizationComponent.generated.h"

class UCustomizationDataAsset;
struct FBodyPartVariant;
class UCustomizationAssetManager;
class USomatotypeDataAsset;
DECLARE_LOG_CATEGORY_EXTERN(LogCustomizationComponent, Log, All);

enum class EItemSlot : uint8;
class UBodyPartAsset;
class UMaterialCustomizationDataAsset;
class UMaterialPackCustomizationDA;
	
struct FAttachedActorChanges
{
	TMap<FName, TArray<TWeakObjectPtr<AActor>>> ActorsToDestroy;
	TArray<FPrimaryAssetId> AssetIdsToLoad;
	TMap<FPrimaryAssetId, FName> AssetIdToSlugMapForLoad;
	TMap<FName, ECustomizationSlotType> SlugToSlotMapForLoad;
};
struct FResolvedVariantInfo
{
    TMap<EBodyPartType, FName> FinalSlotAssignment; 
    TMap<FName, const FBodyPartVariant*> SlugToResolvedVariantMap; 
    TArray<FName> InitialSlugsInTargetState;
};


UCLASS()
class ASYNCCUSTOMISATION_API UCustomizationComponent : public UCharacterComponentBase
{
	GENERATED_BODY()

public:
	UCustomizationComponent();
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSomatotypeLoaded, USomatotypeDataAsset* )
	FOnSomatotypeLoaded OnSomatotypeLoaded;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewCustomizationActorsAttached, const TArray<AActor*>& /* NewAttachedActors */)
	FOnNewCustomizationActorsAttached OnNewCustomizationActorsAttached;

	TMulticastDelegate<void(ABaseCharacter* /*InCharacter*/)> OnInvalidationPipelineCompleted;

	void UpdateFromOwning();

	void EquipItem(const FName& ItemSlug);
	void EquipItems(const TArray<FName>& Items);
	void UnequipItem(const FName& ItemSlug);
	bool RequestUnequipSlot(ECustomizationSlotType InSlotToUnequip);
	
	void ResetAll();
	void HardRefreshAll();

	void SetCustomizationContext(const FCustomizationContextData& InContext);
	
	UFUNCTION(BlueprintPure, Category = "Customization")
	const FCustomizationContextData& GetCurrentCustomizationState();

	const TMap<EBodyPartType, USkeletalMeshComponent*>& GetSkeletals();

	void SetAttachedActorsSimulatePhysics(bool bSimulatePhysics);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemsChanged, const FCustomizationContextData&, NewState);
	UPROPERTY(BlueprintAssignable, Category = "Customization")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	void ApplyCachedMaterialToBodySkinMesh();
protected:

	//Invalidation
	void Invalidate(const FCustomizationContextData& TargetState,
	                const bool bDeffer = true,
	                ECustomizationInvalidationReason ExplicitReason = ECustomizationInvalidationReason::None);

	//Deffer Invalidation context
	ECustomizationInvalidationReason DeferredReason = ECustomizationInvalidationReason::None;

	// 
	TOptional<FCustomizationContextData> DeferredTargetState;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> CachedBodySkinMaterialForCurrentSomatotype;
	
	void LoadAndCacheBodySkinMaterial(const FPrimaryAssetId& SkinMaterialAssetId, ESomatotype ForSomatotype);
	void ApplyFallbackMaterialToBodySkinMesh();
	
	void OnDefferInvalidationTimerExpired();

	//Allowed only to be called in Invalidate(...) method or other Invalidate*
	void InvalidateColoration(FCustomizationContextData& TargetState);
	void InvalidateBodyParts(FCustomizationContextData& TargetState);
	void InvalidateAttachedActors(FCustomizationContextData& TargetState);

	void StartInvalidationTimer(const FCustomizationContextData& TargetState); 
	void CreateTimerIfNeeded();

	// Helpers
	FAttachedActorChanges DetermineAttachedActorChanges(const FCustomizationContextData& CurrentState, const FCustomizationContextData& TargetState);
	void DestroyAttachedActors(const TMap<FName, TArray<TWeakObjectPtr<AActor>>>& ActorsToDestroy);
	void ResetUnusedBodyParts(const FCustomizationContextData& TargetState, const TSet<EBodyPartType>& FinalUsedPartTypes);
	void SpawnAndAttachActorsForItem(
		UCustomizationDataAsset* DataAsset,
		FName ItemSlug,
		const TMap<EBodyPartType, USkeletalMeshComponent*>& CharacterSkeletals,
		ABaseCharacter* CharOwner,
		UWorld* WorldContext,
		TArray<TWeakObjectPtr<AActor>>& OutSpawnedActorPtrs,
		TArray<AActor*>& OutRawSpawnedActorsForEvent);

	TArray<FPrimaryAssetId> CollectRelevantBodyPartAssetIds(
		const FCustomizationContextData& TargetState,
		const FCustomizationContextData& AddedItemsContext,
		const FCustomizationContextData& RemovedItemsContext);

	void RebuildEquippedBodyPartsState(
		FCustomizationContextData& TargetStateToModify,
		const FResolvedVariantInfo& ResolvedVariantData,
		TSet<EBodyPartType>& OutFinalUsedPartTypes,
		TArray<FName>& OutFinalActiveSlugs);

	FResolvedVariantInfo ResolveBodyPartVariantsAndInitialAssignments(
		const FCustomizationContextData& FullTargetContext,
		const TArray<FName>& BodyPartSlugsToResolve,
		const TMap<FName, UBodyPartAsset*>& SlugToAssetMap);

	void ApplyBodyPartMeshesAndSkin(
		FCustomizationContextData& TargetStateContext,
		USomatotypeDataAsset* LoadedSomatotypeDataAsset,
		TSet<EBodyPartType>& FinalUsedPartTypes,
		const TArray<FName>& FinalActiveSlugs,
		const TMap<FName, const FBodyPartVariant*>& SlugToResolvedVariantMap);

	void UpdateMaterialsForBodyPartChanges(FCustomizationContextData& TargetStateToModify, const FResolvedVariantInfo& ResolvedVariantData);

	void ApplyBodySkin(const FCustomizationContextData& TargetState,
	                   const USomatotypeDataAsset* SomatotypeDataAsset,
	                   TSet<EBodyPartType>& FinalUsedPartTypes,
	                   TMap<FName, const FBodyPartVariant*> FinalSlugToVariantMap);
	
	void ApplyMaterialToBodyMesh(ESomatotype Somatotype,
								 USkeletalMeshComponent* BodySkinMeshComp);
	
	void ProcessBodyParts(FCustomizationContextData& TargetStateToModify,
	                             USomatotypeDataAsset* LoadedSomatotypeDataAsset,
	                             TArray<UBodyPartAsset*> LoadedBodyPartAssets);

	void ProcessColoration(FCustomizationContextData& TargetStateToModify, TArray<UObject*> LoadedMaterialAssets);
	
	void HandleInvalidationPipelineCompleted();

	TStrongObjectPtr<UTimerComponent> InvalidationTimer = nullptr;
	
	UPROPERTY()
	FCustomizationContextData CurrentCustomizationState;
	
	UPROPERTY()
	FCustomizationContextData ProcessingTargetState;

	UPROPERTY() /* Contains diff for Invalidation and current state*/
	FCustomizationInvalidationContext InvalidationContext;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool OnlyOneItemInSlot = false;

	UPROPERTY()
	TMap<EBodyPartType, USkeletalMeshComponent*> Skeletals;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	FCounterComponent PendingInvalidationCounter;
private:
	struct FCustomizationDebugInfo
	{
		FCustomizationDebugInfo()
		{
			Reset();
		}

		void Reset()
		{
			EquippedItems = GLOBAL_CONSTANTS::NONE_STRING;
			PendingItems = GLOBAL_CONSTANTS::NONE_STRING;
			SkinInfo = GLOBAL_CONSTANTS::NONE_STRING;
			ActorInfo = GLOBAL_CONSTANTS::NONE_STRING;
			SkinCoverage = GLOBAL_CONSTANTS::NONE_STRING;
		}
		
		FString EquippedItems;
		FString PendingItems;
		FString SkinInfo;
		FString ActorInfo;
		FString SkinCoverage;

		static FString GetBlockText(const FString& Title, const FString& Content) 
		{
			FString BlockText = "-------------------\n";
			BlockText += Title + ":\n";
			BlockText += Content + "\n";
			BlockText += "-------------------";
			return BlockText;
		}

		static FString FormatData(const TMap<FName, EBodyPartType>& Items)
		{
			FString FormattedText;
			for (const auto& Pair : Items)
			{
				FormattedText += FString::Printf(TEXT("%s : %s\n"), *Pair.Key.ToString(), *UEnum::GetValueAsString(Pair.Value));
			}
			return FormattedText;
		}

		static FString FormatData(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& Items)
		{
			FString FormattedText;
			for (const auto& Pair : Items)
			{
				FormattedText += FString::Printf(TEXT("%s : %s\n"), *UEnum::GetValueAsString(Pair.Key), *Pair.Value.ToString());
			}
			return FormattedText;
		}
		
		static FString FormatData(FSkinFlagCombination& Flags)
		{
			return Flags.ToString();
		}
		
		FString FormatData(const FCharacterVFXCustomization& VFX) const
		{
			return VFX.ToString(); 
		}
		
		const float VerticalOffset = 100.0f; 
		const float HorizontalOffset = 100.0f;
		const float OffsetForSecondLine = 65.0f;
		
	};

	FCustomizationDebugInfo DebugInfo;
	FTimerHandle TimerHandle;

	void UpdateDebugInfo();
	void DrawDebugTextBlock(const FVector& Location, const FString& Text, AActor* OwningActor, const FColor& Color);
};