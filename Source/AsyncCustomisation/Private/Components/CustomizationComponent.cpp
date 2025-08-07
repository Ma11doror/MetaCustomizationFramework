#include "AsyncCustomisation/Public/Components/CustomizationComponent.h"

#include "SkeletalMeshMerge.h"
#include "AsyncCustomisation/Public/BaseCharacter.h"
#include "AsyncCustomisation/Public/Components/Core/CustomizationUtilities.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "AsyncCustomisation/Public/Utilities/CommonUtilities.h"
#include "Components/Core/CustomizationItemBase.h"
#include "Components/Core/Assets/SlotMappingAsset.h"
#include "Components/Core/Assets/SomatotypeDataAsset.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utilities/CustomizationSettings.h"
#include "Utilities/MetaGameLib.h"
#include "Utilities/MeshMerger/MeshMergeSubsystem.h"


DEFINE_LOG_CATEGORY(LogCustomizationComponent);

UCustomizationComponent::UCustomizationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentCustomizationState = FCustomizationContextData();
}

void UCustomizationComponent::UpdateFromOwning()
{
	if (!OwningCharacter.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("UpdateFromOwning: OwningCharacter is not valid."));
		return;
	}

	CurrentCustomizationState.Somatotype = OwningCharacter->Somatotype;
	// TODO:: think about it. Where we get info about somatotype? Maybe: recomend user to create meta data with it by himself

	FCustomizationContextData TargetState = CurrentCustomizationState;
	Invalidate(TargetState, false, ECustomizationInvalidationReason::All);
}

void UCustomizationComponent::EquipItem(const FName& ItemSlug)
{
	if (ItemSlug == NAME_None) return;
	
	LoadSlotMappingAndExecute([this, ItemSlug]()
	{
		FCustomizationContextData TargetState = CurrentCustomizationState;
		AddItemToTargetState(ItemSlug, TargetState);
		Invalidate(TargetState, false);
	});
}

void UCustomizationComponent::EquipItems(const TArray<FName>& Items)
{
	if (Items.IsEmpty()) return;

	LoadSlotMappingAndExecute([this, Items]()
	{
		FCustomizationContextData TargetState = CurrentCustomizationState;
		bool bStateChanged = false;

		for (const FName& ItemSlug : Items)
		{
			AddItemToTargetState(ItemSlug, TargetState);
			bStateChanged = true;
		}
        
		if (bStateChanged)
		{
			Invalidate(TargetState, false);
		}
	});
}

void UCustomizationComponent::UnequipItem(const FName& ItemSlug)
{
	if (ItemSlug == NAME_None) return;

	const FPrimaryAssetId ItemCustomizationAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
	if (!ItemCustomizationAssetId.IsValid()) return;
	const auto CustomizationAssetClass = CustomizationUtilities::GetClassForCustomizationAsset(ItemCustomizationAssetId);
	if (!CustomizationAssetClass) return;
	
	//	Copy and then modify TargetState and then Invalidate
	// 1. Copy Current state
	FCustomizationContextData TargetState = CurrentCustomizationState;
	bool bStateChanged = false;

	// 2. 
	if (CustomizationAssetClass->IsChildOf(UCustomizationDataAsset::StaticClass()))
	{
		const CustomizationSlots::FItemRegistryData ItemData = CustomizationSlots::GetItemDataFromRegistry(ItemSlug);
		if (!ItemData.IsValid())
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("UnequipItem: Could not get valid item data from registry for slug %s. Aborting actor unequip."), *ItemSlug.ToString());
			return;
		}
		const FGameplayTag SlotTag = ItemData.InUISlotCategoryTag;
		if (OnlyOneItemInSlot)
		{
			if (const FEquippedItemsInSlotInfo* SlotInfo = TargetState.EquippedCustomizationItemActors.Find(SlotTag))
			{
				if (SlotInfo->EquippedItemActors.Num() > 0 && SlotInfo->EquippedItemActors[0].ItemSlug == ItemSlug)
				{
					if (TargetState.EquippedCustomizationItemActors.Remove(SlotTag) > 0) bStateChanged = true;
				}
			}
		}
		else if (FEquippedItemsInSlotInfo* EquippedItemsInSlotInfo = TargetState.EquippedCustomizationItemActors.Find(SlotTag))
		{
			int32 RemovedCount = EquippedItemsInSlotInfo->EquippedItemActors.RemoveAll(
				[&ItemSlug](const FEquippedItemActorsInfo& EquippedItemActorsInfo) { return EquippedItemActorsInfo.ItemSlug == ItemSlug; });

			if (RemovedCount > 0) bStateChanged = true;

			if (EquippedItemsInSlotInfo->EquippedItemActors.IsEmpty())
			{
				if (TargetState.EquippedCustomizationItemActors.Remove(SlotTag) > 0) bStateChanged = true;
			}
		}
	}
	else if (CustomizationAssetClass->IsChildOf(UMaterialCustomizationDataAsset::StaticClass()) || CustomizationAssetClass->IsChildOf(UMaterialPackCustomizationDA::StaticClass()))
	{
		FGameplayTag SlotForMaterial;
		for (auto& Elem : TargetState.EquippedMaterialsMap)
		{
			if (Elem.Value == ItemSlug)
			{
				SlotForMaterial = Elem.Key;
				break;
			}
		}

		if (SlotForMaterial.IsValid())
		{
			TargetState.EquippedMaterialsMap.Remove(SlotForMaterial);
			bStateChanged = true;
		}
	}
	else if (CustomizationAssetClass->IsChildOf(UBodyPartAsset::StaticClass()))
	{
		FGameplayTag SlotTagOfItemToRemove;

		// Find the slot tag of the item being unequipped
		const FGameplayTag* FoundTag = CurrentCustomizationState.EquippedBodyPartsItems.FindKey(ItemSlug);
		if (FoundTag)
		{
			SlotTagOfItemToRemove = *FoundTag;
		}
		
		if (SlotTagOfItemToRemove.IsValid())
		{
			TargetState.EquippedBodyPartsItems.Remove(SlotTagOfItemToRemove);
			bStateChanged = true;
			UE_LOG(LogCustomizationComponent, Log, TEXT("UnequipItem: Removed BodyPart item '%s' from slot '%s'."), *ItemSlug.ToString(), *SlotTagOfItemToRemove.ToString());

			// If a material is applied to the same slot, it's a skin for our item. Remove it.
			if (TargetState.EquippedMaterialsMap.Contains(SlotTagOfItemToRemove))
			{
				FName MaterialSlug = TargetState.EquippedMaterialsMap.FindAndRemoveChecked(SlotTagOfItemToRemove);
				UE_LOG(LogCustomizationComponent, Log, TEXT("UnequipItem: Automatically removing associated skin '%s' from slot %s."), *MaterialSlug.ToString(), *SlotTagOfItemToRemove.ToString());
				bStateChanged = true;
			}
		}
	}

	// 3.
	if (bStateChanged)
	{
		Invalidate(TargetState, false);
	}
}

bool UCustomizationComponent::RequestUnequipSlot(const FGameplayTag& InSlotToUnequip)
{
	// TODO::
	return false;
}

void UCustomizationComponent::ResetAll()
{
	HardRefreshAll();
	CachedBodySkinMaterialForCurrentSomatotype = nullptr;
	CurrentCustomizationState = FCustomizationContextData();
	if (OwningCharacter.IsValid())
	{
		CurrentCustomizationState.Somatotype = OwningCharacter->Somatotype;
	}

	InvalidationContext.ClearAll();
	Invalidate(CurrentCustomizationState, false, ECustomizationInvalidationReason::All);
}

void UCustomizationComponent::HardRefreshAll()
{
	switch (UCustomizationSettings::Get()->GetMeshMergeMethod())
	{
	case EMeshMergeMethod::SyncMeshMerge:
		{
			// Only reset the main mesh, do not touch SpawnedMeshComponents
			if (OwningCharacter.IsValid() && OwningCharacter->GetMesh())
			{
				OwningCharacter->GetMesh()->SetSkeletalMeshAsset(nullptr);
				// Optionally clear materials if needed
				for (int32 i = 0; i < OwningCharacter->GetMesh()->GetNumMaterials(); ++i)
				{
					OwningCharacter->GetMesh()->SetMaterial(i, nullptr);
				}
			}
			break;
		}

	case EMeshMergeMethod::MasterPose:
		{
			for (auto& [SlotTag, SkeletalComp] : SpawnedMeshComponents)
			{
				if (SkeletalComp)
				{
					SkeletalComp->DestroyComponent();
				}
			}
			SpawnedMeshComponents.Empty();

			for (auto& [SlotType, ActorsInSlot] : CurrentCustomizationState.EquippedCustomizationItemActors)
			{
				for (auto& ActorsInfo : ActorsInSlot.EquippedItemActors)
				{
					for (const auto& ItemRelatedActor : ActorsInfo.ItemRelatedActors)
					{
						if (ItemRelatedActor.IsValid())
						{
							if (AActor* ValidActor = ItemRelatedActor.Get())
							{
								const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
								ValidActor->DetachFromActor(DetachmentTransformRules);
								ValidActor->Destroy();
							}
						}
					}
				}
			}
			break;
		}
	default: UE_LOG(LogTemp, Warning, TEXT("Something went wrong with MergeMethod"));
	}
}

void UCustomizationComponent::SetCustomizationContext(const FCustomizationContextData& InContext)
{
	Invalidate(InContext, false, ECustomizationInvalidationReason::All);
}

const FCustomizationContextData& UCustomizationComponent::GetCurrentCustomizationState()
{
	return CurrentCustomizationState;
}

const TMap<FGameplayTag, TObjectPtr<USkeletalMeshComponent>>& UCustomizationComponent::GetSpawnedMeshComponents()
{
	return SpawnedMeshComponents;
}

void UCustomizationComponent::SetAttachedActorsSimulatePhysics(bool bSimulatePhysics)
{
}

void UCustomizationComponent::Invalidate(const FCustomizationContextData& TargetState,
                                         const bool bDeffer,
                                         ECustomizationInvalidationReason ExplicitReason)
{
	/*
	 * In deffer variant we should call invalidate only once on next tick
	 * This can help avoid multiple invalidation calls while change many properties
	 */
	if (bDeffer)
	{
		if (DeferredTargetState.IsSet() && DeferredTargetState.GetValue() == TargetState)
		{
			ECustomizationInvalidationReason OldDefferReason = DeferredReason;
			EnumAddFlags(DeferredReason, ExplicitReason);
			if (DeferredReason != OldDefferReason)
			{
				UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Deferred invalidation target is same, but reason updated from %s to %s."),
				       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(OldDefferReason)),
				       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(DeferredReason))
				);
			}
			else
			{
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("Invalidate: Deferred invalidation skipped, TargetState (%s items) and Reason (%s) are the same as already deferred one."),
				       *FString::FromInt(TargetState.EquippedBodyPartsItems.Num()),
				       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(DeferredReason))
				);
			}
			if (DeferredTargetState.GetValue() == TargetState && DeferredReason == OldDefferReason)
			{
				return;
			}
		}

		UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Setting up DEFERRED invalidation. Current DefferReason: %s, ExplicitReason for this call: %s."),
		       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(DeferredReason)),
		       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(ExplicitReason))
		);

		EnumAddFlags(DeferredReason, ExplicitReason);
		DeferredTargetState = TargetState;

		FString DeferredItemsStr;
		for (const auto& Pair : DeferredTargetState.GetValue().EquippedBodyPartsItems) DeferredItemsStr += Pair.Key.ToString() + TEXT(" ");
		UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Deferred invalidation requested. DeferredTargetState now has items: [%s]. Final DefferReason for timer: %s."),
		       *DeferredItemsStr,
		       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(DeferredReason))
		);

		StartInvalidationTimer(TargetState);
		return;
	}

	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Starting IMMEDIATE invalidation (called with bDeffer=false)."));
	ProcessingTargetState = TargetState;

	// 1. Get diff (CurrentCustomizationState) and (TargetState)
	FString ContextCurrentSlugs, ProcessingTargetSlugs;
	for (const auto& Pair : CurrentCustomizationState.EquippedBodyPartsItems) ContextCurrentSlugs += Pair.Key.ToString() + TEXT(" ");
	for (const auto& Pair : ProcessingTargetState.EquippedBodyPartsItems) ProcessingTargetSlugs += Pair.Key.ToString() + TEXT(" ");
	UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: PRE-CheckDiff. CurrentCustomizationState Slugs: [%s]. ProcessingTarget Slugs: [%s]"), *ContextCurrentSlugs, *ProcessingTargetSlugs);

	InvalidationContext.CheckDiff(ProcessingTargetState, CurrentCustomizationState);

	UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: POST-CheckDiff. Added BodyParts Num: %d, Removed BodyParts Num: %d"),
	       InvalidationContext.Added.EquippedBodyPartsItems.Num(),
	       InvalidationContext.Removed.EquippedBodyPartsItems.Num()
	);

	ECustomizationInvalidationReason CalculatedReasonFromDiff = InvalidationContext.CalculateReason();

	ECustomizationInvalidationReason CombinedReason = ExplicitReason;
	EnumAddFlags(CombinedReason, CalculatedReasonFromDiff);

	// 2. Calculate final reason
	// if (CombinedReason == ECustomizationInvalidationReason::None)
	if (static_cast<int8>(CombinedReason) == 0)
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: IMMEDIATE invalidation skipped: CombinedReason is None."));

		if (CurrentCustomizationState != ProcessingTargetState)
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("Invalidate: Updating CurrentCustomizationState to ProcessingTargetState as CombinedReason is None, but states differ."));
			CurrentCustomizationState = ProcessingTargetState;
			OnEquippedItemsChanged.Broadcast(CurrentCustomizationState);
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: CurrentCustomizationState IS EQUAL to ProcessingTargetState here (CombinedReason is None)."));
		}

		InvalidationContext.ClearTemporaryContext();

		return;
	}

	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Combined Invalidation Reason: %s (Explicit: %s, Calculated: %s)"),
	       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(CombinedReason)),
	       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(ExplicitReason)),
	       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(CalculatedReasonFromDiff))
	);

    int32 InvalidationStepsLaunched = 0;
    if (EnumHasAnyFlags(CombinedReason, ECustomizationInvalidationReason::Body))
    {
        InvalidationStepsLaunched++;
    }
    if (EnumHasAnyFlags(CombinedReason, ECustomizationInvalidationReason::Actors))
    {
        InvalidationStepsLaunched++;
    }
    if (EnumHasAnyFlags(CombinedReason, ECustomizationInvalidationReason::Skin))
    {
        InvalidationStepsLaunched++;
    }

    if (InvalidationStepsLaunched == 0)
    {
        UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: CombinedReason was not None, but no invalidation steps were identified. Finalizing."));

        if (CurrentCustomizationState != ProcessingTargetState)
        {
            CurrentCustomizationState = ProcessingTargetState;
            OnEquippedItemsChanged.Broadcast(CurrentCustomizationState);
        }
        if (OwningCharacter.IsValid())
        {
            OnInvalidationPipelineCompleted.Broadcast(OwningCharacter.Get());
        }
        InvalidationContext.ClearTemporaryContext();
        DeferredTargetState.Reset();
        DeferredReason = ECustomizationInvalidationReason::None;
        return;
    }
	
    for (int32 i = 0; i < InvalidationStepsLaunched; ++i)
    {
        PendingInvalidationCounter.Push();
    }
    UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Pushed main counter %d times for active invalidation steps."), InvalidationStepsLaunched);
	
	// 3. Execute invalidation steps.
	// Each Invalidate... function now handles its own PendingInvalidationCounter.Push/Pop for its internal async ops.
	// The PendingInvalidationCounter on the UCustomizationComponent is used to track the completion of ALL these async operations.
	// We Push to it before starting any of these potentially async branches.
	// The OnInvalidationPipelineCompleted delegate will fire when the counter reaches zero.


	if (EnumHasAnyFlags(CombinedReason, ECustomizationInvalidationReason::Body))
	{
		InvalidateBodyParts(ProcessingTargetState);
	}
	if (EnumHasAnyFlags(CombinedReason, ECustomizationInvalidationReason::Actors))
	{
		InvalidateAttachedActors(ProcessingTargetState);
	}
	if (EnumHasAnyFlags(CombinedReason, ECustomizationInvalidationReason::Skin))
	{
		InvalidateColoration(ProcessingTargetState);
	}

	FString MapStr;
	for (const auto& Pair : ProcessingTargetState.EquippedBodyPartsItems)
	{
		MapStr += FString::Printf(TEXT(" || [%s: %s] "), *Pair.Key.ToString(), *Pair.Value.ToString());
	}
	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: ProcessingTargetState BodyParts (before async completion by ProcessBodyParts etc.): %s"), *MapStr);

	InvalidationContext.ClearTemporaryContext();
	DeferredTargetState.Reset();
	DeferredReason = ECustomizationInvalidationReason::None;

	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidate: Immediate Invalidation dispatch initiated. Async operations in progress."));
}

void UCustomizationComponent::LoadAndCacheBodySkinMaterial(const FPrimaryAssetId& SkinMaterialAssetId, ESomatotype ForSomatotype)
{
	FPrimaryAssetType AssetType = SkinMaterialAssetId.PrimaryAssetType;
    UE_LOG(LogCustomizationComponent, Log, TEXT("LoadAndCacheBodySkinMaterial: Attempting to load %s for Somatotype %s."), *SkinMaterialAssetId.ToString(), *UEnum::GetValueAsString(ForSomatotype));

    const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);

    if (AssetType.GetName() == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
    {
        UCustomizationAssetManager::StaticAsyncLoadAsset<UMaterialCustomizationDataAsset>(
            SkinMaterialAssetId,
            [this, AssetIdBeingLoaded = SkinMaterialAssetId, SomatotypeAtRequestTime = ForSomatotype, BodySkinSlotTag](UMaterialCustomizationDataAsset* LoadedMaterialAsset)
            {
                if (this == nullptr || !IsValid(this)) return;

                ESomatotype CurrentActiveSomatotype = ProcessingTargetState.Somatotype != ESomatotype::None ? ProcessingTargetState.Somatotype : CurrentCustomizationState.Somatotype;
                if (SomatotypeAtRequestTime != CurrentActiveSomatotype)
                {
                     UE_LOG(LogCustomizationComponent, Log, TEXT("LoadAndCacheBodySkinMaterial: Somatotype changed while loading %s. Discarding."), *AssetIdBeingLoaded.ToString());
                     return;
                }

                if (LoadedMaterialAsset)
                {
                    if (LoadedMaterialAsset->TargetItemSlot == BodySkinSlotTag && LoadedMaterialAsset->IndexWithApplyingMaterial.Contains(0))
                    {
                        CachedBodySkinMaterialForCurrentSomatotype = LoadedMaterialAsset->IndexWithApplyingMaterial[0];
                        UE_LOG(LogCustomizationComponent, Log, TEXT("LoadAndCacheBodySkinMaterial: Cached skin material %s for %s."), *AssetIdBeingLoaded.ToString(), *UEnum::GetValueAsString(SomatotypeAtRequestTime));
                        ApplyCachedMaterialToBodySkinMesh();
                    }
                    else
                    {
                        UE_LOG(LogCustomizationComponent, Warning, TEXT("LoadAndCacheBodySkinMaterial: Loaded %s for %s is not for BodySkin or has no material."), *AssetIdBeingLoaded.ToString(), *UEnum::GetValueAsString(SomatotypeAtRequestTime));
                        CachedBodySkinMaterialForCurrentSomatotype = nullptr;
                        ApplyFallbackMaterialToBodySkinMesh();
                    }
                }
                else
                {
                    UE_LOG(LogCustomizationComponent, Warning, TEXT("LoadAndCacheBodySkinMaterial: Failed to load %s for %s."), *AssetIdBeingLoaded.ToString(), *UEnum::GetValueAsString(SomatotypeAtRequestTime));
                    CachedBodySkinMaterialForCurrentSomatotype = nullptr;
                    ApplyFallbackMaterialToBodySkinMesh();
                }
            });
    }
    else if (AssetType.GetName() == GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType)
    {
        UCustomizationAssetManager::StaticAsyncLoadAsset<UMaterialPackCustomizationDA>(
            SkinMaterialAssetId,
            [this, AssetIdBeingLoaded = SkinMaterialAssetId, SomatotypeAtRequestTime = ForSomatotype, BodySkinSlotTag](UMaterialPackCustomizationDA* LoadedMaterialPack)
            {
                if (this == nullptr || !IsValid(this)) return;
                
                ESomatotype CurrentActiveSomatotype = ProcessingTargetState.Somatotype != ESomatotype::None ? ProcessingTargetState.Somatotype : CurrentCustomizationState.Somatotype;
                if (SomatotypeAtRequestTime != CurrentActiveSomatotype)
                {
                     UE_LOG(LogCustomizationComponent, Log, TEXT("LoadAndCacheBodySkinMaterial: Somatotype changed while loading pack %s. Discarding."), *AssetIdBeingLoaded.ToString());
                     return;
                }

                if (LoadedMaterialPack)
                {
                    UMaterialInterface* FoundMaterial = nullptr;
                    for (UMaterialCustomizationDataAsset* MatAsset : LoadedMaterialPack->MaterialAsset.MaterialCustomizations)
                    {
                        if (MatAsset && MatAsset->TargetItemSlot == BodySkinSlotTag && MatAsset->IndexWithApplyingMaterial.Contains(0))
                        {
                            FoundMaterial = MatAsset->IndexWithApplyingMaterial[0];
                            break;
                        }
                    }

                    if (FoundMaterial)
                    {
                        CachedBodySkinMaterialForCurrentSomatotype = FoundMaterial;
                        UE_LOG(LogCustomizationComponent, Log, TEXT("LoadAndCacheBodySkinMaterial: Cached skin material from pack %s for %s."), *AssetIdBeingLoaded.ToString(), *UEnum::GetValueAsString(SomatotypeAtRequestTime));
                        ApplyCachedMaterialToBodySkinMesh();
                    }
                    else
                    {
                        UE_LOG(LogCustomizationComponent, Warning, TEXT("LoadAndCacheBodySkinMaterial: No BodySkin material in pack %s for %s."), *AssetIdBeingLoaded.ToString(), *UEnum::GetValueAsString(SomatotypeAtRequestTime));
                        CachedBodySkinMaterialForCurrentSomatotype = nullptr;
                        ApplyFallbackMaterialToBodySkinMesh();
                    }
                }
                else
                {
                    UE_LOG(LogCustomizationComponent, Warning, TEXT("LoadAndCacheBodySkinMaterial: Failed to load pack %s for %s."), *AssetIdBeingLoaded.ToString(), *UEnum::GetValueAsString(SomatotypeAtRequestTime));
                    CachedBodySkinMaterialForCurrentSomatotype = nullptr;
                    ApplyFallbackMaterialToBodySkinMesh();
                }
            });
    }
    else
    {
        UE_LOG(LogCustomizationComponent, Error, TEXT("LoadAndCacheBodySkinMaterial: AssetId %s has unsupported type %s for somatotype %s skin material."), *SkinMaterialAssetId.ToString(), *AssetType.ToString(), *UEnum::GetValueAsString(ForSomatotype));
        CachedBodySkinMaterialForCurrentSomatotype = nullptr;
        ApplyFallbackMaterialToBodySkinMesh();
    }
}

void UCustomizationComponent::ApplyCachedMaterialToBodySkinMesh()
{
	const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);
	TObjectPtr<USkeletalMeshComponent> BodySkinMeshCompPtr = SpawnedMeshComponents.FindRef(BodySkinSlotTag);
	
	if (USkeletalMeshComponent* BodySkinMeshComp = BodySkinMeshCompPtr.Get())
	{
		if(BodySkinMeshComp->GetSkeletalMeshAsset())
		{
			if (CachedBodySkinMaterialForCurrentSomatotype)
			{
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("ApplyCachedMaterialToBodySkinMesh: Applying cached material %s to %s."), *CachedBodySkinMaterialForCurrentSomatotype->GetName(), *BodySkinMeshComp->GetName());
				int32 NumMaterials = BodySkinMeshComp->GetNumMaterials();
				for (int32 i = 0; i < NumMaterials; ++i)
				{
					BodySkinMeshComp->SetMaterial(i, CachedBodySkinMaterialForCurrentSomatotype);
				}
			}
			else
			{
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("ApplyCachedMaterialToBodySkinMesh: No cached skin material. Applying fallback."));
				ApplyFallbackMaterialToBodySkinMesh();
			}
		}
	}
}

void UCustomizationComponent::ApplyFallbackMaterialToBodySkinMesh()
{
	const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);
	TObjectPtr<USkeletalMeshComponent> BodySkinMeshCompPtr = SpawnedMeshComponents.FindRef(BodySkinSlotTag);

	if (USkeletalMeshComponent* BodySkinMeshComp = BodySkinMeshCompPtr.Get())
	{
		if(BodySkinMeshComp->GetSkeletalMeshAsset())
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("ApplyFallbackMaterialToBodySkinMesh: Applying fallback (mesh default) material to %s."), *BodySkinMeshComp->GetName());
			const USkeletalMesh* MeshAsset = BodySkinMeshComp->GetSkeletalMeshAsset();
			if (MeshAsset)
			{
				const auto& DefaultMaterials = MeshAsset->GetMaterials();
				for (int32 i = 0; i < BodySkinMeshComp->GetNumMaterials(); ++i)
				{
					if (DefaultMaterials.IsValidIndex(i))
					{
						BodySkinMeshComp->SetMaterial(i, DefaultMaterials[i].MaterialInterface);
					}
					else
					{
						BodySkinMeshComp->SetMaterial(i, nullptr);
					}
				}
			}
		}
	}
}

void UCustomizationComponent::OnDefferInvalidationTimerExpired()
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("Deferred invalidation timer expired."));
	if (DeferredTargetState.IsSet())
	{
		FCustomizationContextData StateToInvalidate = DeferredTargetState.GetValue();
		ECustomizationInvalidationReason ReasonForInvalidation = DeferredReason;

		DeferredTargetState.Reset();
		DeferredReason = ECustomizationInvalidationReason::None;
		Invalidate(StateToInvalidate, false, ReasonForInvalidation);
		// Invalidate(DeferredTargetState.GetValue(), false, DefferReason);
	}
	else
	{
		DeferredReason = ECustomizationInvalidationReason::None;
		UE_LOG(LogCustomizationComponent, Warning, TEXT("Deferred invalidation timer expired, but no TargetState was stored."));
	}
}

void UCustomizationComponent::InvalidateColoration(FCustomizationContextData& TargetState)
{
	TArray<FPrimaryAssetId> MaterialAssetIdsToLoad;
	for (const auto& Pair : TargetState.EquippedMaterialsMap)
	{
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Pair.Value);
		if (AssetId.IsValid())
		{
			MaterialAssetIdsToLoad.AddUnique(AssetId);
		}
	}
	
	if (MaterialAssetIdsToLoad.IsEmpty())
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: No new materials to load, proceeding to process potential removals."));
		ProcessColoration(this->ProcessingTargetState, {});
		PendingInvalidationCounter.Pop(); // Pop the counter for the InvalidateColoration step itself.
		return;
	}

	TSharedPtr<bool> bHasPoppedForThisStep = MakeShared<bool>(false);
	auto FinalizeAndPopOnce = [this, bHasPoppedForThisStep](const FString& ReasonMessage) {
		if (bHasPoppedForThisStep.IsValid() && !(*bHasPoppedForThisStep))
		{
			*bHasPoppedForThisStep = true;
			UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: %s. Popping counter."), *ReasonMessage);
			PendingInvalidationCounter.Pop();
		}
	};

	UCustomizationAssetManager::StaticAsyncLoadAssetList<UMaterialCustomizationDataAsset>(
		MaterialAssetIdsToLoad,
		[this, FinalizeAndPopOnce](TArray<UMaterialCustomizationDataAsset*> LoadedAssets) mutable
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Async load completed. Loaded %d material assets."), LoadedAssets.Num());
			
			TArray<UObject*> LoadedObjects;
			LoadedObjects.Reserve(LoadedAssets.Num());
			for(UPrimaryDataAsset* Asset : LoadedAssets)
			{
				LoadedObjects.Add(Asset);
			}
			
			ProcessColoration(this->ProcessingTargetState, LoadedObjects);

			FinalizeAndPopOnce(TEXT("Finished processing coloration."));
		}
	);
}

void UCustomizationComponent::InvalidateBodyParts(FCustomizationContextData& TargetState)
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateBodyParts: Starting invalidation. Somatotype: %s."), *UEnum::GetValueAsString(TargetState.Somatotype));

	const FPrimaryAssetId SomatotypeAssetId = CustomizationUtilities::GetSomatotypeAssetId(TargetState.Somatotype);
	if (!SomatotypeAssetId.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("InvalidateBodyParts: Invalid SomatotypeAssetId for Somatotype %s. Aborting body part invalidation."), *UEnum::GetValueAsString(TargetState.Somatotype));
		CachedBodySkinMaterialForCurrentSomatotype = nullptr; 
		ApplyFallbackMaterialToBodySkinMesh(); 
		return;
	}

	FCustomizationContextData TargetStateAtInvalidationStart = TargetState;
	
	TSharedPtr<bool> bHasPoppedForThisStep = MakeShared<bool>(false);

	auto FinalizeAndPopOnce = [this, bHasPoppedForThisStep](const FString& ReasonMessage) {
		if (bHasPoppedForThisStep.IsValid() && !(*bHasPoppedForThisStep))
		{
			*bHasPoppedForThisStep = true;
			UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateBodyParts: %s. Popping counter."), *ReasonMessage);
			PendingInvalidationCounter.Pop();
		}
		else if (bHasPoppedForThisStep.IsValid() && *bHasPoppedForThisStep)
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateBodyParts: Attempted to Pop counter again for: %s. Already popped this step."), *ReasonMessage);
		}
	};
	
	UCustomizationAssetManager::GetCustomizationAssetManager()->AsyncLoadAsset<USomatotypeDataAsset>(SomatotypeAssetId,
                 [this, CapturedTargetState = TargetStateAtInvalidationStart, FinalizeAndPopOnce](USomatotypeDataAsset* LoadedSomatotypeDataAsset)
                 {
	                 if (!LoadedSomatotypeDataAsset)
	                 {
		                 FinalizeAndPopOnce(FString::Printf(TEXT("Failed to load SomatotypeDataAsset for %s."), *UEnum::GetValueAsString(CapturedTargetState.Somatotype)));
		                 CachedBodySkinMaterialForCurrentSomatotype = nullptr;
		                 ApplyFallbackMaterialToBodySkinMesh();
		                 return;
	                 }

                     UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateBodyParts: SomatotypeDataAsset %s loaded."), *LoadedSomatotypeDataAsset->GetPrimaryAssetId().ToString());

                 	// Initiate loading of the default skin material associated with this somatotype
	                 const FPrimaryAssetId DefaultSkinMaterialAssetId = UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(CapturedTargetState.Somatotype);
	                 if (DefaultSkinMaterialAssetId.IsValid())
	                 {
		                 // Clear previous cache before loading new one
		                 CachedBodySkinMaterialForCurrentSomatotype = nullptr;
		                 LoadAndCacheBodySkinMaterial(DefaultSkinMaterialAssetId, CapturedTargetState.Somatotype);
	                 }
	                 else
	                 {
	                 	// Apply fallback if a skin mesh is set without a cached material
		                 UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateBodyParts: No DefaultSkinMaterialAssetId for Somatotype %s. Skin material cache will be empty."), *UEnum::GetValueAsString(CapturedTargetState.Somatotype));
		                 CachedBodySkinMaterialForCurrentSomatotype = nullptr;
		                 ApplyFallbackMaterialToBodySkinMesh(); 
	                 }
                 	
                     TArray<FPrimaryAssetId> AllRelevantItemAssetIds = CollectRelevantBodyPartAssetIds(
                         CapturedTargetState,
                         this->InvalidationContext.Added,
                         this->InvalidationContext.Removed
                     );

                     if (AllRelevantItemAssetIds.IsEmpty())
                     {
                         ProcessBodyParts(this->ProcessingTargetState, LoadedSomatotypeDataAsset, {});
                         FinalizeAndPopOnce(TEXT("Finished (no relevant body part assets were loaded)."));
                         return;
                     }

                     UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateBodyParts: Requesting async load for %d BodyPartAssets."), AllRelevantItemAssetIds.Num());
                     UCustomizationAssetManager::StaticAsyncLoadAssetList<UBodyPartAsset>(
                         AllRelevantItemAssetIds,
                         [this, LoadedSomatotypeDataAsset, FinalizeAndPopOnce]
                     (TArray<UBodyPartAsset*> LoadedBodyPartAssets)
                         {
                             UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateBodyParts: Async load of %d BodyPartAssets completed."), LoadedBodyPartAssets.Num());
                             ProcessBodyParts(this->ProcessingTargetState, LoadedSomatotypeDataAsset, LoadedBodyPartAssets);
                             FinalizeAndPopOnce(TEXT("Finished (all body part assets processed)."));
                         });
                 });
}

void UCustomizationComponent::ProcessBodyParts(FCustomizationContextData& TargetStateToModify,
                                               USomatotypeDataAsset* LoadedSomatotypeDataAsset,
                                               TArray<UBodyPartAsset*> LoadedBodyPartAssets)
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("ProcessBodyParts: Processing with %d loaded BodyPartAssets. Initial TargetStateToModify.EquippedBodyPartsItems.Num: %d"),
	       LoadedBodyPartAssets.Num(), TargetStateToModify.EquippedBodyPartsItems.Num());

	if (!LoadedSomatotypeDataAsset)
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("ProcessBodyParts: SomatotypeDataAsset is null. Cannot process body parts."));
		return;
	}

	// 1. Create a map of ItemSlug to UBodyPartAsset for quick lookups
	TMap<FName, UBodyPartAsset*> SlugToAssetMap;
	SlugToAssetMap.Reserve(LoadedBodyPartAssets.Num());
	for (UBodyPartAsset* BodyPartAsset : LoadedBodyPartAssets)
	{
		if (BodyPartAsset)
		{
			SlugToAssetMap.Add(BodyPartAsset->GetPrimaryAssetId().PrimaryAssetName, BodyPartAsset);
		}
	}

	// 2. 
	TArray<FName> BodyPartSlugsToResolve;
	TargetStateToModify.EquippedBodyPartsItems.GenerateValueArray(BodyPartSlugsToResolve);

	// 3. Resolve variants for equipped body parts and determine slot assignments
	// Pass the full TargetStateToModify as the context for variant matching.
	FResolvedVariantInfo ResolvedVariantData = ResolveBodyPartVariantsAndInitialAssignments(
		TargetStateToModify,
		BodyPartSlugsToResolve, 
		SlugToAssetMap
	);

	// 4. Update material assignments if body part configurations have changed
	UpdateMaterialsForBodyPartChanges(TargetStateToModify, ResolvedVariantData);

	// 5. Rebuild the EquippedBodyPartsItems map in TargetStateToModify based on resolved variants
	TSet<FGameplayTag> FinalUsedSlotTags; 
	TArray<FName> FinalActiveSlugs;
	RebuildEquippedBodyPartsState(TargetStateToModify, ResolvedVariantData, FinalUsedSlotTags, FinalActiveSlugs);
    
	// 6. Apply actual skeletal meshes, the body skin, and reset unused parts
	ApplyBodyPartMeshesAndSkin(
		TargetStateToModify,
		LoadedSomatotypeDataAsset,
		FinalUsedSlotTags,
		FinalActiveSlugs,
		ResolvedVariantData.SlugToResolvedVariantMap
	);

	// 7. 
	DebugInfo.EquippedItems = TargetStateToModify.GetItemsList();
	OnSomatotypeLoaded.Broadcast(LoadedSomatotypeDataAsset);
	UE_LOG(LogCustomizationComponent, Log, TEXT("ProcessBodyParts: Finished. Final TargetStateToModify.EquippedBodyPartsItems.Num: %d"), TargetStateToModify.EquippedBodyPartsItems.Num());
}

void UCustomizationComponent::ProcessColoration(FCustomizationContextData& TargetStateToModify, TArray<UObject*> LoadedMaterialAssets)
{
	const EMeshMergeMethod MergeMethod = UCustomizationSettings::Get()->GetMeshMergeMethod();
	if (MergeMethod != EMeshMergeMethod::MasterPose)
	{
		ApplyMergedMaterials();
		return;
	}
	
	// 1.
	TMap<FName, FGameplayTag> SlugToSlotTagMap;
	for (UObject* LoadedAsset : LoadedMaterialAssets)
	{
		FName Slug = NAME_None;
		FGameplayTag SlotTag;

		if (auto MaterialAsset = Cast<UMaterialCustomizationDataAsset>(LoadedAsset))
		{
			Slug = MaterialAsset->GetPrimaryAssetId().PrimaryAssetName;
			SlotTag = MaterialAsset->TargetItemSlot;
		}
		else if (auto MaterialPack = Cast<UMaterialPackCustomizationDA>(LoadedAsset))
		{
			Slug = MaterialPack->GetPrimaryAssetId().PrimaryAssetName;
			if (MaterialPack->MaterialAsset.MaterialCustomizations.Num() > 0 && MaterialPack->MaterialAsset.MaterialCustomizations[0])
			{
				SlotTag = MaterialPack->MaterialAsset.MaterialCustomizations[0]->TargetItemSlot;
			}
		}

		if (Slug != NAME_None && SlotTag.IsValid())
		{
			SlugToSlotTagMap.Add(Slug, SlotTag);
		}
	}

	// 2.
	TMap<FGameplayTag, FName> FinalSlotAssignment;
	for (const auto& Pair : TargetStateToModify.EquippedMaterialsMap)
	{
		const FName& Slug = Pair.Value;
		if (const FGameplayTag* FoundSlotTag = SlugToSlotTagMap.Find(Slug))
		{
			FinalSlotAssignment.Add(*FoundSlotTag, Slug);
		}
	}

	TargetStateToModify.EquippedMaterialsMap = FinalSlotAssignment;

	// 4. 
	for (UObject* LoadedAsset : LoadedMaterialAssets)
	{
		if (auto MaterialAsset = Cast<UMaterialCustomizationDataAsset>(LoadedAsset))
		{
			const FName Slug = MaterialAsset->GetPrimaryAssetId().PrimaryAssetName;
			const FName* FoundSlug = TargetStateToModify.EquippedMaterialsMap.Find(MaterialAsset->TargetItemSlot);

			if (FoundSlug && *FoundSlug == Slug)
			{
				if (MaterialAsset->bApplyOnBodyPart)
				{
					if (USkeletalMeshComponent* TargetMesh = CreateOrGetMeshComponentForSlot(MaterialAsset->TargetItemSlot))
					{
						CustomizationUtilities::SetMaterialOnMesh(MaterialAsset, TargetMesh);
					}
				}
			}
		}
		else if (auto MaterialPack = Cast<UMaterialPackCustomizationDA>(LoadedAsset))
		{
			const FName Slug = MaterialPack->GetPrimaryAssetId().PrimaryAssetName;
			if (!MaterialPack->MaterialAsset.MaterialCustomizations.IsEmpty() && MaterialPack->MaterialAsset.MaterialCustomizations[0])
			{
				const FGameplayTag PackSlotTag = MaterialPack->MaterialAsset.MaterialCustomizations[0]->TargetItemSlot;
				const FName* FoundSlug = TargetStateToModify.EquippedMaterialsMap.Find(PackSlotTag);

				if (FoundSlug && *FoundSlug == Slug)
				{
					for (UMaterialCustomizationDataAsset* MaterialInPack : MaterialPack->MaterialAsset.MaterialCustomizations)
					{
						if (MaterialInPack && MaterialInPack->bApplyOnBodyPart)
						{
							if (USkeletalMeshComponent* TargetMesh = CreateOrGetMeshComponentForSlot(MaterialInPack->TargetItemSlot))
							{
								CustomizationUtilities::SetMaterialOnMesh(MaterialInPack, TargetMesh);
							}
						}
					}
				}
			}
		}
	}
	// 5. Reset materials for slots that no longer have a custom skin applied
	for (const auto& BodyPartPair : TargetStateToModify.EquippedBodyPartsItems)
	{
		const FGameplayTag& SlotTag = BodyPartPair.Key;
		const FName& BodyPartSlug = BodyPartPair.Value;

		if (!TargetStateToModify.EquippedMaterialsMap.Contains(SlotTag))
		{
			// This slot should have its default materials.
			if (auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager())
			{
				FPrimaryAssetId BodyPartAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(BodyPartSlug);
				if (UBodyPartAsset* BodyPartAsset = AssetManager->LoadPrimaryAsset<UBodyPartAsset>(BodyPartAssetId))
				{
					TArray<FPrimaryAssetId> AllEquippedItemAssetIdsInContext;
					for (const FName& Slug : TargetStateToModify.GetEquippedSlugs())
					{
						if (FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug); AssetId.IsValid())
						{
							AllEquippedItemAssetIdsInContext.Add(AssetId);
						}
					}
					
					const FBodyPartVariant* Variant = BodyPartAsset->GetMatchedVariant(AllEquippedItemAssetIdsInContext);
					if (USkeletalMeshComponent* TargetMesh = CreateOrGetMeshComponentForSlot(SlotTag))
					{
						UE_LOG(LogCustomizationComponent, Log, TEXT("ProcessColoration: Slot %s has no custom skin. Resetting to default materials."), *SlotTag.ToString());
						CustomizationUtilities::ApplyDefaultMaterials(TargetMesh, Variant);
					}
				}
			}
		}
	}
}

void UCustomizationComponent::ApplyMergedMaterials()
{
	if (!OwningCharacter.IsValid() || !OwningCharacter->GetMesh() || !OwningCharacter->GetMesh()->GetSkeletalMeshAsset())
	{
		return;
	}

	USkeletalMeshComponent* TargetMeshComponent = OwningCharacter->GetMesh();
	USkeletalMesh* CurrentMesh = TargetMeshComponent->GetSkeletalMeshAsset();

	// First, apply all default materials from the merged mesh itself to ensure a clean state
	// TODO:: useless action, check if we have skins for slots somehow at this stage and skip?
	const auto& DefaultMaterials = CurrentMesh->GetMaterials();
	for (int32 i = 0; i < DefaultMaterials.Num(); ++i)
	{
		TargetMeshComponent->SetMaterial(i, DefaultMaterials[i].MaterialInterface);
	}
    
	// Now, override with custom skins where applicable
	auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
	if (!AssetManager)
	{
		return;
	}
    
	for (const auto& MaterialPair : ProcessingTargetState.EquippedMaterialsMap)
	{
		const FGameplayTag& SlotTag = MaterialPair.Key;
		const FName& SkinSlug = MaterialPair.Value;

		for (const auto& MapPair : MergedMaterialMap)
		{
			if (MapPair.Value == SlotTag)
			{
				const int32 MaterialIndex = MapPair.Key;
				FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(SkinSlug);

				if (auto* MaterialAsset = AssetManager->LoadPrimaryAsset<UMaterialCustomizationDataAsset>(SkinAssetId))
				{
					if(MaterialAsset->IndexWithApplyingMaterial.Contains(0))
					{
						UMaterialInterface* SkinMaterial = MaterialAsset->IndexWithApplyingMaterial[0];
						TargetMeshComponent->SetMaterial(MaterialIndex, SkinMaterial);
						UE_LOG(LogCustomizationComponent, Log, TEXT("Applied skin %s to merged mesh at index %d for slot %s"), *SkinSlug.ToString(), MaterialIndex, *SlotTag.ToString());
					}
				}
			}
		}
	}
}

void UCustomizationComponent::HandleInvalidationPipelineCompleted()
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("HandleInvalidationPipelineCompleted: All async invalidation operations finished."));
	if (CurrentCustomizationState != ProcessingTargetState)
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("HandleInvalidationPipelineCompleted: Updating CurrentCustomizationState from final ProcessingTargetState."));
		CurrentCustomizationState = ProcessingTargetState;
		OnEquippedItemsChanged.Broadcast(CurrentCustomizationState);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("HandleInvalidationPipelineCompleted: CurrentCustomizationState already matches final ProcessingTargetState. No update needed."));
	}
	if (OwningCharacter.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("HandleInvalidationPipelineCompleted: Broadcasting OnInvalidationPipelineCompleted delegate."));
		OnInvalidationPipelineCompleted.Broadcast(OwningCharacter.Get());
	}
}

void UCustomizationComponent::ApplyBodySkin(const FCustomizationContextData& TargetState,
											const USomatotypeDataAsset* SomatotypeDataAsset,
											TSet<FGameplayTag>& FinalUsedSlotTags,
											TMap<FName, const FBodyPartVariant*> FinalSlugToVariantMap)
{
	UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying Body Skin..."));

	FSkinFlagCombination SkinVisibilityFlags;
	for (const auto& Pair : FinalSlugToVariantMap)
	{
		if (Pair.Value)
		{
			SkinVisibilityFlags.AddFlag(Pair.Value->SkinCoverageFlags.FlagMask);
		}
	}

	auto SkinMeshVariant = SkinVisibilityFlags.GetMatch(SomatotypeDataAsset->SkinAssociation, SkinVisibilityFlags.FlagMask);
	const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);

	if (SkinMeshVariant)
	{
		UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying Body Skin Mesh based on flags: %s"), *SkinVisibilityFlags.ToString());
		FinalUsedSlotTags.Emplace(BodySkinSlotTag);
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, SkinMeshVariant->BodyPartSkeletalMesh, nullptr, BodySkinSlotTag);
		DebugInfo.SkinCoverage = DebugInfo.FormatData(SkinVisibilityFlags);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("No matching Body Skin Mesh found for flags: %s. Resetting skin mesh."), *SkinVisibilityFlags.ToString());
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, nullptr, BodySkinSlotTag);
		DebugInfo.SkinCoverage = FString::Printf(TEXT("No Match: %s"), *SkinVisibilityFlags.ToString());
	}
}

void UCustomizationComponent::ResetUnusedBodyParts(const TSet<FGameplayTag>& FinalUsedSlotTags)
{
    const EMeshMergeMethod MergeMethod = UCustomizationSettings::Get()->GetMeshMergeMethod();
	if (MergeMethod == EMeshMergeMethod::MasterPose)
	{
		// --- OLD MASTER POSE PIPELINE ---
		UE_LOG(LogCustomizationComponent, Verbose, TEXT("Resetting unused body parts (not in FinalUsedSlotTags)..."));
		TArray<FGameplayTag> TagsToDestroy;
		for (const auto& [SlotTag, SkeletalComp] : SpawnedMeshComponents)
		{
			if (SkeletalComp && !FinalUsedSlotTags.Contains(SlotTag))
			{
				TagsToDestroy.Add(SlotTag);
			}
		}
		for (const FGameplayTag& Tag : TagsToDestroy)
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("Destroying component for unused SlotTag: %s"), *Tag.ToString());
			USkeletalMeshComponent* CompToDestroy = SpawnedMeshComponents.FindAndRemoveChecked(Tag).Get();
			if (IsValid(CompToDestroy))
			{
				CompToDestroy->DestroyComponent();
			}
		}
	}
}

void UCustomizationComponent::SpawnAndAttachActorsForItem(UCustomizationDataAsset* DataAsset,
														  FName ItemSlug,
														  ABaseCharacter* CharOwner,
														  UWorld* WorldContext,
														  TArray<TWeakObjectPtr<AActor>>& OutSpawnedActorPtrs,
														  TArray<AActor*>& OutRawSpawnedActorsForEvent)
{
	// 1. 
	if (!IsValid(DataAsset))
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("SpawnAndAttachActorsForItem: Invalid DataAsset for ItemSlug: %s."), *ItemSlug.ToString());
		return;
	}
	if (!CharOwner || !CharOwner->GetMesh())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("SpawnAndAttachActorsForItem: Invalid OwningCharacter or its Mesh for ItemSlug: %s."), *ItemSlug.ToString());
		return;
	}
	if (!WorldContext)
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("SpawnAndAttachActorsForItem: WorldContext is null for ItemSlug: %s."), *ItemSlug.ToString());
		return;
	}

	UE_LOG(LogCustomizationComponent, Verbose, TEXT("SpawnAndAttachActorsForItem: Spawning actors for ItemSlug: %s (Asset: %s)"), *ItemSlug.ToString(), *DataAsset->GetPrimaryAssetId().ToString());

	const TArray<FCustomizationComplect>& SuitableComplects = DataAsset->CustomizationComplect;
	if (SuitableComplects.IsEmpty())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("SpawnAndAttachActorsForItem: CustomizationComplect array is empty in asset %s for item %s."),
		       *DataAsset->GetPrimaryAssetId().ToString(), *ItemSlug.ToString());
		return;
	}

	OutSpawnedActorPtrs.Reserve(OutSpawnedActorPtrs.Num() + SuitableComplects.Num());
	OutRawSpawnedActorsForEvent.Reserve(OutRawSpawnedActorsForEvent.Num() + SuitableComplects.Num());

	// 2. Iterate through complects and spawn actors
	for (const FCustomizationComplect& Complect : SuitableComplects)
	{
		if (!Complect.ActorClass)
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("SpawnAndAttachActorsForItem: Invalid ActorClass in Complect for item %s"), *ItemSlug.ToString());
			continue;
		}

		// 3. Determine attach target
		USkeletalMeshComponent* AttachTarget = nullptr;
		const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);
		if (TObjectPtr<USkeletalMeshComponent> BodySkinComp = SpawnedMeshComponents.FindRef(BodySkinSlotTag))
		{
			AttachTarget = BodySkinComp;
		}
		else
		{
			AttachTarget = CharOwner->GetMesh();
			if (AttachTarget)
			{
				UE_LOG(LogCustomizationComponent, Warning, TEXT("SpawnAndAttachActorsForItem: SpawnedMeshComponents does not contain a valid BodySkin mesh component for item %s. Using OwningCharacter->GetMesh() (%s)."), *ItemSlug.ToString(), *AttachTarget->GetName());
			}
			else
			{
				UE_LOG(LogCustomizationComponent, Error, TEXT("SpawnAndAttachActorsForItem: Fallback OwningCharacter->GetMesh() is also null for item %s."), *ItemSlug.ToString());
				continue;
			}
		}

		if (!AttachTarget) // Should be redundant due to above checks, but as a safeguard
		{
			UE_LOG(LogCustomizationComponent, Error, TEXT("SpawnAndAttachActorsForItem: No valid SkeletalMeshComponent found to attach actor for item %s. Cannot spawn."), *ItemSlug.ToString());
			continue;
		}

		// 4. Spawn actor and attach
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = CharOwner;

		AActor* SpawnedActor = WorldContext->SpawnActor<AActor>(Complect.ActorClass, FTransform::Identity, Params);
		UE_LOG(LogTemp, Error, TEXT("[LEAK TEST] Spawned: %s, Outer: %s, Owner: %s, World: %s"),
		       *SpawnedActor->GetName(),
		       *GetNameSafe(SpawnedActor->GetOuter()),
		       *GetNameSafe(SpawnedActor->GetOwner()),
		       *GetNameSafe(SpawnedActor->GetWorld()));
		
		if (!IsValid(SpawnedActor))
		{
			UE_LOG(LogCustomizationComponent, Error, TEXT("SpawnAndAttachActorsForItem: Failed to spawn actor of class %s for item %s"), *Complect.ActorClass->GetName(), *ItemSlug.ToString());
			continue;
		}

		OutSpawnedActorPtrs.Add(SpawnedActor);
		OutRawSpawnedActorsForEvent.Add(SpawnedActor);
		SpawnedActor->Tags.Add(GLOBAL_CONSTANTS::CustomizationTag);


		SpawnedActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Complect.SocketName);
		SpawnedActor->SetActorRelativeTransform(Complect.RelativeTransform);

		UE_LOG(LogCustomizationComponent, Verbose, TEXT("SpawnAndAttachActorsForItem: Spawned and attached actor %s to %s at socket %s for item %s"),
		       *SpawnedActor->GetName(), *AttachTarget->GetName(), *Complect.SocketName.ToString(), *ItemSlug.ToString());
	}
}

TArray<FPrimaryAssetId> UCustomizationComponent::CollectRelevantBodyPartAssetIds(const FCustomizationContextData& TargetState, const FCustomizationContextData& AddedItemsContext, const FCustomizationContextData& RemovedItemsContext)
{
	TSet<FPrimaryAssetId> RelevantAssetIdsSet;
	UE_LOG(LogCustomizationComponent, Log, TEXT("CollectRelevantBodyPartAssetIds: Building RelevantAssetIdsSet..."));

	// 1. Add assets from newly equipped body parts in the diff
	for (const auto& Pair : AddedItemsContext.EquippedBodyPartsItems)
	{
		const FName& Slug = Pair.Value; // Value is the slug
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		if (AssetId.IsValid())
		{
			RelevantAssetIdsSet.Add(AssetId);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("CollectRelevantBodyPartAssetIds: Added slug %s (AssetID: %s) from AddedItemsContext."), *Slug.ToString(), *AssetId.ToString());
		}
	}

	// 2. Add assets from removed body parts in the diff
	for (const auto& Pair : RemovedItemsContext.EquippedBodyPartsItems)
	{
		const FName& Slug = Pair.Value; // Value is the slug
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		if (AssetId.IsValid())
		{
			RelevantAssetIdsSet.Add(AssetId);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("CollectRelevantBodyPartAssetIds: Added slug %s (AssetID: %s) from RemovedItemsContext."), *Slug.ToString(), *AssetId.ToString());
		}
	}

	// 3. Add assets currently specified in the target state's body part items
	for (const auto& Pair : TargetState.EquippedBodyPartsItems)
	{
		const FName& Slug = Pair.Value; // Value is the slug
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		if (AssetId.IsValid())
		{
			RelevantAssetIdsSet.Add(AssetId);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("CollectRelevantBodyPartAssetIds: Added slug %s (AssetID: %s) from TargetState itself."), *Slug.ToString(), *AssetId.ToString());
		}
	}

	TArray<FPrimaryAssetId> AllRelevantItemAssetIds = RelevantAssetIdsSet.Array();
	UE_LOG(LogCustomizationComponent, Log, TEXT("CollectRelevantBodyPartAssetIds: Collected %d unique relevant asset IDs."), AllRelevantItemAssetIds.Num());
	return AllRelevantItemAssetIds;
}

void UCustomizationComponent::RebuildEquippedBodyPartsState(FCustomizationContextData& TargetStateToModify,
															const FResolvedVariantInfo& ResolvedVariantData,
															TSet<FGameplayTag>& OutFinalUsedSlotTags,
															TArray<FName>& OutFinalActiveSlugs)
{
	// 1. Clear the current body part assignments in the target state
	TargetStateToModify.EquippedBodyPartsItems.Empty();
	UE_LOG(LogCustomizationComponent, Log, TEXT("RebuildEquippedBodyPartsState: Cleared TargetState.EquippedBodyPartsItems. Rebuilding..."));

	OutFinalUsedSlotTags.Empty();
	OutFinalActiveSlugs.Empty();
	OutFinalActiveSlugs.Reserve(ResolvedVariantData.FinalSlotAssignment.Num());

	// 2. Rebuild based on the final slot assignments from variant resolution
	for (const auto& Pair : ResolvedVariantData.FinalSlotAssignment)
	{
		FGameplayTag SlotTag = Pair.Key;
		FName Slug = Pair.Value;

		const FBodyPartVariant* const* FoundVariantPtr = ResolvedVariantData.SlugToResolvedVariantMap.Find(Slug);

		if (FoundVariantPtr && (*FoundVariantPtr))
		{
			TargetStateToModify.EquippedBodyPartsItems.Add(SlotTag, Slug);
			OutFinalUsedSlotTags.Add(SlotTag);
			OutFinalActiveSlugs.Add(Slug);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("RebuildEquippedBodyPartsState: Final assignment for SlotTag %s is Slug %s."), *SlotTag.ToString(), *Slug.ToString());
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("RebuildEquippedBodyPartsState: Variant for winning slug %s (Tag %s) not found in SlugToResolvedVariantMap. This should ideally not happen if slug is in FinalSlotAssignment. Skipping rebuild for this item."), *Slug.ToString(), *SlotTag.ToString());
		}
	}
	UE_LOG(LogCustomizationComponent, Log, TEXT("RebuildEquippedBodyPartsState: Rebuilt TargetState.EquippedBodyPartsItems. New Num: %d"), TargetStateToModify.EquippedBodyPartsItems.Num());
}

FResolvedVariantInfo UCustomizationComponent::ResolveBodyPartVariantsAndInitialAssignments(const FCustomizationContextData& FullTargetContext,
                                                                                           const TArray<FName>& BodyPartSlugsToResolve,
                                                                                           const TMap<FName, UBodyPartAsset*>& SlugToAssetMap)
{
	FResolvedVariantInfo Result;
	Result.InitialSlugsInTargetState = BodyPartSlugsToResolve;

	// 1. Get all slugs from any equipped item type in FullTargetContext for variant context
	TArray<FPrimaryAssetId> AllEquippedItemAssetIdsInContext;
	AllEquippedItemAssetIdsInContext.Reserve(FullTargetContext.GetEquippedSlugs().Num());
	for (const FName& Slug : FullTargetContext.GetEquippedSlugs())
	{
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		if (AssetId.IsValid())
		{
			AllEquippedItemAssetIdsInContext.Add(AssetId);
		}
	}

	UE_LOG(LogCustomizationComponent, Log, TEXT("ResolveBodyPartVariants: Resolving for %d body part slugs with %d items in full context."), BodyPartSlugsToResolve.Num(), AllEquippedItemAssetIdsInContext.Num());

	// 2. Resolve variants for each body part slug we're interested in
	for (const FName& ItemSlug : BodyPartSlugsToResolve)
	{
		const UBodyPartAsset* const* FoundAssetPtr = SlugToAssetMap.Find(ItemSlug);
		if (!FoundAssetPtr || !IsValid(*FoundAssetPtr))
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("ResolveBodyPartVariants: BodyPartAsset not found or invalid for slug '%s' in SlugToAssetMap. Skipping."), *ItemSlug.ToString());
			continue;
		}
		const UBodyPartAsset* BodyPartAsset = *FoundAssetPtr;
		
		if (BodyPartAsset->TargetItemSlot.IsValid())
		{
			Result.SlugToSlotTagMap.Add(ItemSlug, BodyPartAsset->TargetItemSlot);
		}
		
		const FBodyPartVariant* MatchedVariant = BodyPartAsset->GetMatchedVariant(AllEquippedItemAssetIdsInContext);

		if (MatchedVariant && MatchedVariant->IsValid())
		{
			FGameplayTag ResolvedSlotTag = BodyPartAsset->TargetItemSlot; // Get the slot from the asset itself.
			if (ResolvedSlotTag.IsValid())
			{
				Result.FinalSlotAssignment.Add(ResolvedSlotTag, ItemSlug); 
				Result.SlugToResolvedVariantMap.Add(ItemSlug, MatchedVariant);
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("ResolveBodyPartVariants: Slug '%s' (Asset: %s) resolved to SlotTag: %s."), *ItemSlug.ToString(), *BodyPartAsset->GetPrimaryAssetId().ToString(), *ResolvedSlotTag.ToString());
			}
			else
			{
				UE_LOG(LogCustomizationComponent, Warning, TEXT("ResolveBodyPartVariants: Matched variant for slug '%s' has an invalid SlotTag. Skipping assignment."), *ItemSlug.ToString());
			}
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("ResolveBodyPartVariants: No valid variant found for slug '%s' using current equipment context. Skipping assignment."), *ItemSlug.ToString());
		}
	}
	return Result;
}

void UCustomizationComponent::ApplyBodyPartsMasterPose(USomatotypeDataAsset* LoadedSomatotypeDataAsset, const TMap<FName, const FBodyPartVariant*>& SlugToResolvedVariantMap, TSet<FGameplayTag>& FinalUsedSlotTags, const FCustomizationContextData& TargetStateContext)
{
	// 1. Apply Body Skin Mesh
	FSkinFlagCombination SkinVisibilityFlags;
	for (const auto& Pair : SlugToResolvedVariantMap)
	{
		if (Pair.Value)
		{
			SkinVisibilityFlags.AddFlag(Pair.Value->SkinCoverageFlags.FlagMask);
		}
	}
	auto SkinMeshVariant = SkinVisibilityFlags.GetMatch(LoadedSomatotypeDataAsset->SkinAssociation, SkinVisibilityFlags.FlagMask);
	const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);

	if (SkinMeshVariant)
	{
		UE_LOG(LogCustomizationComponent, Verbose, TEXT("[MasterPose] Applying Body Skin Mesh based on flags: %s"), *SkinVisibilityFlags.ToString());
		FinalUsedSlotTags.Emplace(BodySkinSlotTag);
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, SkinMeshVariant->BodyPartSkeletalMesh, nullptr, BodySkinSlotTag);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("[MasterPose] No matching Body Skin Mesh found for flags: %s. Resetting skin mesh."), *SkinVisibilityFlags.ToString());
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, nullptr, BodySkinSlotTag);
	}
    
	// 2. Apply Body Part Meshes
	for (const auto& Pair : TargetStateContext.EquippedBodyPartsItems)
	{
		const FGameplayTag& SlotTag = Pair.Key;
		const FName& Slug = Pair.Value;
        
		if (const FBodyPartVariant* const* FoundVariantPtr = SlugToResolvedVariantMap.Find(Slug))
		{
			if (const FBodyPartVariant* Variant = *FoundVariantPtr)
			{
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("[MasterPose] Applying BodyPart '%s' to slot '%s'"), *Slug.ToString(), *SlotTag.ToString());
				CustomizationUtilities::SetBodyPartSkeletalMesh(this, Variant->BodyPartSkeletalMesh, Variant, SlotTag);
			}
		}
	}

	// 3. Reset unused parts
	ResetUnusedBodyParts(FinalUsedSlotTags);
    
	// 4. Finalize
	HandleInvalidationPipelineCompleted();
}

void UCustomizationComponent::ApplyBodyPartsMeshMerge(FCustomizationContextData& TargetStateContext, USomatotypeDataAsset* LoadedSomatotypeDataAsset, const TArray<FName>& FinalActiveSlugs, const TMap<FName, const FBodyPartVariant*>& SlugToResolvedVariantMap)
{
	TArray<FMeshToMergeData> MeshesToMergeData;

    // Main skin mesh (required)
    USkeletalMesh* SkinMesh = nullptr;
    {
        FSkinFlagCombination SkinVisibilityFlags;
        for (const auto& Pair : SlugToResolvedVariantMap)
        {
            if (Pair.Value)
            {
                SkinVisibilityFlags.AddFlag(Pair.Value->SkinCoverageFlags.FlagMask);
            }
        }
        auto SkinMeshVariant = SkinVisibilityFlags.GetMatch(LoadedSomatotypeDataAsset->SkinAssociation, SkinVisibilityFlags.FlagMask);
        if (SkinMeshVariant && SkinMeshVariant->BodyPartSkeletalMesh)
        {
        	SkinMesh = SkinMeshVariant->BodyPartSkeletalMesh;
        	FMeshToMergeData SkinData;
        	SkinData.SkeletalMesh = SkinMesh;
        	SkinData.SlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);
        	MeshesToMergeData.Add(SkinData);
        }
        else
        {
            UE_LOG(LogCustomizationComponent, Error, TEXT("[MESH MERGE] No valid skin mesh found for current somatotype and flags! Aborting merge."));
            HandleInvalidationPipelineCompleted();
            return;
        }
    }

    // Body parts (attachments)
    for (const FName& Slug : FinalActiveSlugs)
    {
        const FBodyPartVariant* const* FoundVariantPtr = SlugToResolvedVariantMap.Find(Slug);
        if (FoundVariantPtr && (*FoundVariantPtr) && (*FoundVariantPtr)->BodyPartSkeletalMesh)
        {
            USkeletalMesh* PartMesh = (*FoundVariantPtr)->BodyPartSkeletalMesh;
            FGameplayTag SlotTag = TargetStateContext.EquippedBodyPartsItems.FindKey(Slug) ? *TargetStateContext.EquippedBodyPartsItems.FindKey(Slug) : FGameplayTag();
            if (PartMesh && PartMesh != SkinMesh)
            {
            	FMeshToMergeData PartData;
            	PartData.SkeletalMesh = PartMesh;
            	if (const FGameplayTag* SlotTagPtr = TargetStateContext.EquippedBodyPartsItems.FindKey(Slug))
            	{
            		PartData.SlotTag = *SlotTagPtr;
            	}
            	MeshesToMergeData.Add(PartData);
            }
        }
    }
    
    if (!SkinMesh)
    {
        if (OwningCharacter.IsValid() && OwningCharacter->GetMesh())
        {
            OwningCharacter->GetMesh()->SetSkeletalMeshAsset(nullptr);
        }
        HandleInvalidationPipelineCompleted();
        return;
    }
    
	if (MeshesToMergeData.IsEmpty())
	{
		if (OwningCharacter.IsValid() && OwningCharacter->GetMesh())
		{
			OwningCharacter->GetMesh()->SetSkeletalMeshAsset(nullptr);
		}
		HandleInvalidationPipelineCompleted();
		return;
	}
	
	UMeshMergeSubsystem::MergeMeshesWithSettings(GetWorld(), MeshesToMergeData, &MergedMaterialMap, FOnMeshMergeCompleteDelegate::CreateUObject(this, &UCustomizationComponent::OnMergeCompleted));
}

void UCustomizationComponent::ApplyBodyPartMeshesAndSkin(FCustomizationContextData& TargetStateContext,
                                                         USomatotypeDataAsset* LoadedSomatotypeDataAsset,
                                                         TSet<FGameplayTag>& FinalUsedSlotTags,
                                                         const TArray<FName>& FinalActiveSlugs,
                                                         const TMap<FName,
                                                                    const FBodyPartVariant*>& SlugToResolvedVariantMap)
{
	const EMeshMergeMethod MergeMethod = UCustomizationSettings::Get()->GetMeshMergeMethod();

	if (MergeMethod == EMeshMergeMethod::MasterPose)
	{
		ApplyBodyPartsMasterPose(LoadedSomatotypeDataAsset, SlugToResolvedVariantMap, FinalUsedSlotTags, TargetStateContext);
	}
	else 
	{
		ApplyBodyPartsMeshMerge(TargetStateContext, LoadedSomatotypeDataAsset, FinalActiveSlugs, SlugToResolvedVariantMap);
	}
}

void UCustomizationComponent::UpdateMaterialsForBodyPartChanges(FCustomizationContextData& TargetStateToModify, const FResolvedVariantInfo& ResolvedVariantData)
{
	TSet<FGameplayTag> AffectedSlotTagsForMaterialReset;
	TMap<FGameplayTag, FName> OriginalSlotsForInitialSlugs;

	// Build a map of what slot each initial slug *would* have occupied.
	for (const FName& OldSlug : ResolvedVariantData.InitialSlugsInTargetState)
	{
		if (const FGameplayTag* SlotTagPtr = ResolvedVariantData.SlugToSlotTagMap.Find(OldSlug))
		{
			if(SlotTagPtr->IsValid())
			{
				OriginalSlotsForInitialSlugs.Add(*SlotTagPtr, OldSlug);
			}
		}
	}

	// Now compare the original state with the final assignment to see which slots have changed owners.
	for (const auto& OriginalPair : OriginalSlotsForInitialSlugs)
	{
		const FGameplayTag& SlotTag = OriginalPair.Key;
		const FName& OriginalSlug = OriginalPair.Value;
		
		const FName* FinalSlugInSlot = ResolvedVariantData.FinalSlotAssignment.Find(SlotTag);

		// If the slot is now empty OR is occupied by a different item, it's affected.
		if (!FinalSlugInSlot || *FinalSlugInSlot != OriginalSlug)
		{
			AffectedSlotTagsForMaterialReset.Add(SlotTag);
		}
	}

	// Also check for slots that are newly occupied but were empty before.
	for (const auto& FinalPair : ResolvedVariantData.FinalSlotAssignment)
	{
		if (!OriginalSlotsForInitialSlugs.Contains(FinalPair.Key))
		{
			AffectedSlotTagsForMaterialReset.Add(FinalPair.Key);
		}
	}

	if (AffectedSlotTagsForMaterialReset.Num() > 0)
	{
		FString AffectedTagsStr;
		for (FGameplayTag Tag : AffectedSlotTagsForMaterialReset) AffectedTagsStr += Tag.ToString() + TEXT(" ");
		UE_LOG(LogCustomizationComponent, Log, TEXT("UpdateMaterialsForBodyPartChanges: BodyPart slots potentially affected material-wise: [%s]. Clearing associated materials."), *AffectedTagsStr.TrimEnd());

		TArray<FGameplayTag> MaterialSlotsToRemove;
		MaterialSlotsToRemove.Reserve(TargetStateToModify.EquippedMaterialsMap.Num());

		for (const auto& MaterialPair : TargetStateToModify.EquippedMaterialsMap)
		{
			if (MaterialPair.Key.IsValid() && AffectedSlotTagsForMaterialReset.Contains(MaterialPair.Key))
			{
				MaterialSlotsToRemove.Add(MaterialPair.Key);
			}
		}

		if (MaterialSlotsToRemove.Num() > 0)
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("UpdateMaterialsForBodyPartChanges: Removing %d materials from EquippedMaterialsMap."), MaterialSlotsToRemove.Num());
			for (const FGameplayTag& SlotToRemove : MaterialSlotsToRemove)
			{
				FName RemovedSlug = TargetStateToModify.EquippedMaterialsMap.FindAndRemoveChecked(SlotToRemove);
				UE_LOG(LogCustomizationComponent, Log, TEXT("UpdateMaterialsForBodyPartChanges: Removed material %s from slot %s."), *RemovedSlug.ToString(), *SlotToRemove.ToString());
			}
		}
	}
}

void UCustomizationComponent::InvalidateAttachedActors(FCustomizationContextData& TargetState)
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: Starting invalidation based on TargetState..."));

	// 1. Determine what actors to destroy and what assets to load
	FAttachedActorChanges ActorChanges = DetermineAttachedActorChanges(CurrentCustomizationState, TargetState);

	// 2. Early exit if no changes are needed
	if (ActorChanges.AssetIdsToLoad.IsEmpty() && ActorChanges.ActorsToDestroy.IsEmpty())
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: No actors to destroy and no new assets to load. Invalidation complete."));
		PendingInvalidationCounter.Pop();
		return;
	}

	// 3. Handle case: Only destroy actors, no new assets to load
	if (ActorChanges.AssetIdsToLoad.IsEmpty() && !ActorChanges.ActorsToDestroy.IsEmpty())
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: Only destroying actors, no new assets to load."));
		DestroyAttachedActors(ActorChanges.ActorsToDestroy);
		DebugInfo.ActorInfo = TargetState.GetActorsList();
		PendingInvalidationCounter.Pop();
		UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: Finished (only destroyed actors)."));
		return;
	}

	TSharedPtr<bool> bHasPoppedForThisStep = MakeShared<bool>(false);
	auto FinalizeAndPopOnce = [this, bHasPoppedForThisStep](const FString& ReasonMessage)
	{
		if (bHasPoppedForThisStep.IsValid() && !(*bHasPoppedForThisStep))
		{
			*bHasPoppedForThisStep = true;
			UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: %s. Popping counter."), *ReasonMessage);
			PendingInvalidationCounter.Pop();
		}
		else if (bHasPoppedForThisStep.IsValid() && *bHasPoppedForThisStep)
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateAttachedActors: Attempted to Pop counter again for: %s. Already popped this step."), *ReasonMessage);
		}
	};
	
	// 4. Request loading of CustomizationDataAssets if there are assets to load
	UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: Requesting async load for %d CustomizationDataAssets."), ActorChanges.AssetIdsToLoad.Num());

	UCustomizationAssetManager::StaticAsyncLoadAssetList<UCustomizationDataAsset>(
		ActorChanges.AssetIdsToLoad,
		[this, &TargetStateRef = TargetState, LocalActorChanges = ActorChanges, FinalizeAndPopOnce]
	(TArray<UCustomizationDataAsset*> LoadedAssets) mutable
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateAttachedActors: Async load completed. Loaded %d CustomizationDataAssets."), LoadedAssets.Num());

			// 5. Destroy old actors (if any were marked for destruction)
			if (!LocalActorChanges.ActorsToDestroy.IsEmpty())
			{
				DestroyAttachedActors(LocalActorChanges.ActorsToDestroy);
			}

			// 6. Spawn new actors based on loaded assets
			// Collect all spawned actors for a single broadcast
			TMap<FName, TArray<TWeakObjectPtr<AActor>>> NewSpawnedActorsMap;
			TArray<AActor*> AllRawSpawnedActorsForEvent; 

			for (UCustomizationDataAsset* DataAsset : LoadedAssets)
			{
				if (!IsValid(DataAsset))
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateAttachedActors: Encountered an invalid CustomizationDataAsset in LoadedAssets."));
					continue;
				}

				const FPrimaryAssetId LoadedAssetId = DataAsset->GetPrimaryAssetId();
				const FName* ItemSlugPtr = LocalActorChanges.AssetIdToSlugMapForLoad.Find(LoadedAssetId);

				if (!ItemSlugPtr)
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateAttachedActors: Loaded asset %s not found in AssetIdToSlugMapForLoad. Skipping spawn."), *LoadedAssetId.ToString());
					continue;
				}
				const FName ItemSlug = *ItemSlugPtr;
				// const ECustomizationSlotType* SlotTypePtr = LocalActorChanges.SlugToSlotMapForLoad.Find(ItemSlug); // If needed later

				TArray<TWeakObjectPtr<AActor>> SpawnedActorPtrsForItem;
				TArray<AActor*> RawSpawnedActorsForItemEvent;

				SpawnAndAttachActorsForItem(
					DataAsset,
					ItemSlug,
					OwningCharacter.Get(),
					GetWorld(),
					SpawnedActorPtrsForItem,
					RawSpawnedActorsForItemEvent
				);

				if (!SpawnedActorPtrsForItem.IsEmpty())
				{
					NewSpawnedActorsMap.Add(ItemSlug, SpawnedActorPtrsForItem);
				}
				if (!RawSpawnedActorsForItemEvent.IsEmpty())
				{
					AllRawSpawnedActorsForEvent.Append(RawSpawnedActorsForItemEvent);
				}
			}

			// 7. Perform post-spawn operations (physics, events)
			if (!AllRawSpawnedActorsForEvent.IsEmpty())
			{
				TArray<AActor*> ActorsForTimer = AllRawSpawnedActorsForEvent;
				GetWorld()->GetTimerManager().SetTimerForNextTick([this, ActorsForTimer]() mutable // Capture this if needed for CustomizationItemBase
				{
					for (AActor* SActor : ActorsForTimer)
					{
						if (IsValid(SActor))
						{
							if (ACustomizationItemBase* CustomizationItemBase = Cast<ACustomizationItemBase>(SActor))
							{
								CustomizationItemBase->SetItemSimulatePhysics(true);
							}
						}
					}
				});
				OnNewCustomizationActorsAttached.Broadcast(AllRawSpawnedActorsForEvent);
			}

			// 8. Update TargetStateRef with the newly spawned actor references
			for (auto& Pair : TargetStateRef.EquippedCustomizationItemActors)
			{
				// ECustomizationSlotType SlotType = Pair.Key;
				FEquippedItemsInSlotInfo& ItemsInSlot = Pair.Value;
				for (FEquippedItemActorsInfo& ActorInfo : ItemsInSlot.EquippedItemActors)
				{
					if (TArray<TWeakObjectPtr<AActor>>* FoundSpawnedActors = NewSpawnedActorsMap.Find(ActorInfo.ItemSlug))
					{
						ActorInfo.ItemRelatedActors = *FoundSpawnedActors;
						UE_LOG(LogCustomizationComponent, Verbose, TEXT("InvalidateAttachedActors: Updated TargetStateRef.ItemRelatedActors for slug %s with %d actors."), *ActorInfo.ItemSlug.ToString(), FoundSpawnedActors->Num());
					}
				}
			}

			// 9. Finalize and update debug information
			DebugInfo.ActorInfo = TargetStateRef.GetActorsList();
			FinalizeAndPopOnce(TEXT("Finished processing attached actors."));
		});
}

void UCustomizationComponent::StartInvalidationTimer(const FCustomizationContextData& TargetState)
{
	CreateTimerIfNeeded();
	InvalidationTimer->Start(3.0f, 1, UTimerComponent::ETimerMode::FrameDelay);
}

void UCustomizationComponent::CreateTimerIfNeeded()
{
	if (!InvalidationTimer.IsValid())
	{
		InvalidationTimer = TStrongObjectPtr(NewObject<UTimerComponent>(this, "CustomizationInvalidationTimer"));
		InvalidationTimer->OnTimerElapsed.AddUObject(this, &ThisClass::OnDefferInvalidationTimerExpired);
	}
}

USkeletalMeshComponent* UCustomizationComponent::CreateOrGetMeshComponentForSlot(const FGameplayTag& SlotTag)
{
	if (!SlotTag.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("CreateOrGetMeshComponentForSlot: Invalid SlotTag provided."));
		return nullptr;
	}

	if (TObjectPtr<USkeletalMeshComponent> const* ExistingCompPtr = SpawnedMeshComponents.Find(SlotTag))
	{
		if (ExistingCompPtr->Get())
		{
			return ExistingCompPtr->Get();
		}
	}

	if (!OwningCharacter.IsValid() || !OwningCharacter->GetMesh())
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("CreateOrGetMeshComponentForSlot: OwningCharacter or its root mesh is invalid. Cannot create component."));
		return nullptr;
	}

	const FName CompName = FName(*FString::Printf(TEXT("CustomizationComp_%s"), *SlotTag.GetTagName().ToString().Replace(TEXT("."), TEXT("_"))));
	USkeletalMeshComponent* NewSkelComp = NewObject<USkeletalMeshComponent>(GetOwner(), CompName);
	if (NewSkelComp)
	{
		NewSkelComp->SetupAttachment(OwningCharacter->GetMesh());
		NewSkelComp->RegisterComponent();
		NewSkelComp->SetLeaderPoseComponent(OwningCharacter->GetMesh());
		SpawnedMeshComponents.Add(SlotTag, NewSkelComp);
		UE_LOG(LogCustomizationComponent, Log, TEXT("CreateOrGetMeshComponentForSlot: Created and registered new SkeletalMeshComponent '%s' for slot '%s'"), *CompName.ToString(), *SlotTag.ToString());
		return NewSkelComp;
	}

	UE_LOG(LogCustomizationComponent, Error, TEXT("CreateOrGetMeshComponentForSlot: Failed to create new SkeletalMeshComponent for slot '%s'"), *SlotTag.ToString());
	return nullptr;
}

FAttachedActorChanges UCustomizationComponent::DetermineAttachedActorChanges(const FCustomizationContextData& CurrentState, const FCustomizationContextData& TargetState)
{
	FAttachedActorChanges Changes;

	// 1. Determine actors to destroy by comparing slugs in each slot
	for (const auto& CurrentSlotPair : CurrentState.EquippedCustomizationItemActors)
	{
		const FGameplayTag& SlotTag = CurrentSlotPair.Key;
		const FEquippedItemsInSlotInfo& CurrentItemsInSlot = CurrentSlotPair.Value;
		const FEquippedItemsInSlotInfo* TargetItemsInSlot = TargetState.EquippedCustomizationItemActors.Find(SlotTag);

		for (const FEquippedItemActorsInfo& CurrentActorInfo : CurrentItemsInSlot.EquippedItemActors)
		{
			bool bFoundInTarget = false;
			if (TargetItemsInSlot)
			{
				if (TargetItemsInSlot->EquippedItemActors.ContainsByPredicate(
					[&CurrentActorInfo](const FEquippedItemActorsInfo& TargetActorInfo) { return TargetActorInfo.ItemSlug == CurrentActorInfo.ItemSlug; }))
				{
					bFoundInTarget = true;
				}
			}

			if (!bFoundInTarget)
			{
				Changes.ActorsToDestroy.Add(CurrentActorInfo.ItemSlug, CurrentActorInfo.ItemRelatedActors); 
			}
		}
	}

	// 2. Determine new assets to load for spawning
	for (const auto& TargetSlotPair : TargetState.EquippedCustomizationItemActors)
	{
		const FGameplayTag& SlotTag = TargetSlotPair.Key;
		const FEquippedItemsInSlotInfo& TargetItemsInSlot = TargetSlotPair.Value;
		const FEquippedItemsInSlotInfo* CurrentItemsInSlot = CurrentState.EquippedCustomizationItemActors.Find(SlotTag);

		for (const FEquippedItemActorsInfo& TargetActorInfo : TargetItemsInSlot.EquippedItemActors)
		{
			bool bFoundInCurrent = false;
			if (CurrentItemsInSlot)
			{
				if (CurrentItemsInSlot->EquippedItemActors.ContainsByPredicate(
					[&TargetActorInfo](const FEquippedItemActorsInfo& CurrentActorInfo) { return CurrentActorInfo.ItemSlug == TargetActorInfo.ItemSlug; }))
				{
					bFoundInCurrent = true;
				}
			}

			if (!bFoundInCurrent)
			{
				// This item is new in the target state for this slot, so we need to load its asset.
				FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(TargetActorInfo.ItemSlug);
				if (AssetId.IsValid())
				{
					Changes.AssetIdsToLoad.AddUnique(AssetId);
					Changes.AssetIdToSlugMapForLoad.Add(AssetId, TargetActorInfo.ItemSlug);
					Changes.SlugToSlotMapForLoad.Add(TargetActorInfo.ItemSlug, SlotTag);
				}
				else
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("DetermineAttachedActorChanges: Invalid AssetId for ItemSlug: %s. Cannot load for spawning."), *TargetActorInfo.ItemSlug.ToString());
				}
			}
		}
	}
	return Changes;
}

void UCustomizationComponent::DestroyAttachedActors(const TMap<FName, TArray<TWeakObjectPtr<AActor>>>& ActorsToDestroy)
{
	for (const auto& Pair : ActorsToDestroy)
	{
		const TArray<TWeakObjectPtr<AActor>>& ActorsList = Pair.Value;

		for (const TWeakObjectPtr<AActor>& ActorPtr : ActorsList)
		{
			if (AActor* ValidActor = ActorPtr.Get())
			{
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("DestroyAttachedActors: Destroying actor %s"), *ValidActor->GetName());
				ValidActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				ValidActor->Destroy();
			}
		}
	}
}

void UCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ABaseCharacter>(GetOwner());

	PendingInvalidationCounter.OnTriggered.AddUObject(this, &UCustomizationComponent::HandleInvalidationPipelineCompleted);

	if (IsActive() && OwningCharacter.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("BeginPlay: Updating from owning character."));
		UpdateFromOwning();
	}
	else if (!OwningCharacter.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("BeginPlay: OwningCharacter is not valid or not a BaseCharacter."));
	}

#if WITH_EDITOR
	if (UCustomizationSettings::Get()->GetEnableDebug())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			UpdateDebugInfo();
		}, 0.1f, true, 0.f);
	}
#endif
}

void UCustomizationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InvalidationTimer.IsValid())
	{
		InvalidationTimer->OnTimerElapsed.Clear();
		InvalidationTimer->Stop();
		InvalidationTimer = nullptr;
	}
#if WITH_EDITOR
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
#endif

	DeferredTargetState.Reset();
	DeferredReason = ECustomizationInvalidationReason::None;

	PendingInvalidationCounter.OnTriggered.RemoveAll(this);
	
	CachedBodySkinMaterialForCurrentSomatotype = nullptr;
	CurrentCustomizationState.ClearAttachedActors();
	ProcessingTargetState.ClearAttachedActors(); 
    
	InvalidationContext.ClearAll();
    
	Super::EndPlay(EndPlayReason);
}

void UCustomizationComponent::AddItemToTargetState(const FName& ItemSlug, FCustomizationContextData& InOutTargetState)
{
	if (!LoadedSlotMapping)
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("AddItemToTargetState called before SlotMappingAsset was loaded."));
		return;
	}

	const FPrimaryAssetId ItemCustomizationAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
	if (!ItemCustomizationAssetId.IsValid()) return;
	
	const auto CustomizationAssetClass = CustomizationUtilities::GetClassForCustomizationAsset(ItemCustomizationAssetId);
	if (!CustomizationAssetClass) return;

	auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
	ensure(AssetManager);
	
	const CustomizationSlots::FItemRegistryData ItemData = CustomizationSlots::GetItemDataFromRegistry(ItemSlug);
	if (!ItemData.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("AddItemToTargetState: Could not get valid registry data for item %s."), *ItemSlug.ToString());
		return;
	}
	const FGameplayTag& UISlotTag = ItemData.InUISlotCategoryTag;
	const EItemType ItemType = ItemData.ItemType;
	
	if (ItemType == EItemType::Skin)
	{
		// skin
		FGameplayTag ItemTechnicalSlot = CommonUtilities::GetItemSlotTagForSlug(ItemSlug, SlugToSlotTagCache);
		if (ItemTechnicalSlot.IsValid())
        {
            InOutTargetState.EquippedMaterialsMap.Add(ItemTechnicalSlot, ItemSlug);
			UE_LOG(LogCustomizationComponent, Log, TEXT("AddItemToTargetState: Applying skin '%s' to technical slot '%s'."), *ItemSlug.ToString(), *ItemTechnicalSlot.ToString());
        }
        else
        {
            UE_LOG(LogCustomizationComponent, Warning, TEXT("AddItemToTargetState: Could not resolve a technical slot for SKIN item %s."), *ItemSlug.ToString());
        }
	}
	else // base item (BodyPart, Actor, etc.)
	{
		// 1. Clear all technical slots associated with this UI slot.
		if (const FGameplayTagContainer* TechnicalSlotsToClear = LoadedSlotMapping->UISlotToTechnicalSlots.Find(UISlotTag))
		{
			for (const FGameplayTag& TechSlot : *TechnicalSlotsToClear)
			{
				InOutTargetState.EquippedBodyPartsItems.Remove(TechSlot);
				InOutTargetState.EquippedMaterialsMap.Remove(TechSlot);
				InOutTargetState.EquippedCustomizationItemActors.Remove(TechSlot);
			}
		}

		// 2. Add the new item to its correct map.
		if (CustomizationAssetClass->IsChildOf(UCustomizationDataAsset::StaticClass()))
		{
			const FGameplayTag SlotTag = ItemData.InUISlotCategoryTag;
			if (OnlyOneItemInSlot)
			{
				const FEquippedItemActorsInfo EquippedItemActorsInfo = {ItemSlug, {}};
				const FEquippedItemsInSlotInfo EquippedItemsInSlotInfo = {{EquippedItemActorsInfo}};
				InOutTargetState.EquippedCustomizationItemActors.Emplace(SlotTag, EquippedItemsInSlotInfo);
			}
			else
			{
				InOutTargetState.ReplaceOrAddSpawnedActors(ItemSlug, SlotTag, {});
			}
		}
		else
		{
			FGameplayTag ItemTechnicalSlot = CommonUtilities::GetItemSlotTagForSlug(ItemSlug, SlugToSlotTagCache);
			if (ItemTechnicalSlot.IsValid())
			{
				if (CustomizationAssetClass->IsChildOf(UBodyPartAsset::StaticClass()))
				{
					InOutTargetState.EquippedBodyPartsItems.Add(ItemTechnicalSlot, ItemSlug);
				}
				// Note: Applying a material directly is now handled by the EItemType::Skin case above.
				// This branch is now only for BodyParts and Actors.
			}
			else
			{
				UE_LOG(LogCustomizationComponent, Warning, TEXT("AddItemToTargetState: Could not resolve a technical slot for non-actor item %s."), *ItemSlug.ToString());
			}
		}
	}
}

void UCustomizationComponent::LoadSlotMappingAndExecute(TFunction<void()> OnComplete)
{
	if (LoadedSlotMapping)
	{
		if(OnComplete) OnComplete();
		return;
	}

	if (SlotMappingAsset.IsNull())
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("SlotMappingAsset is not set in CustomizationComponent. Aborting operation."));
		return;
	}

	FStreamableManager& StreamableManager = UCustomizationAssetManager::Get().GetStreamableManager();
	StreamableManager.RequestAsyncLoad(SlotMappingAsset.ToSoftObjectPath(), [this, OnComplete]()
	{
		LoadedSlotMapping = SlotMappingAsset.Get();
		if (LoadedSlotMapping)
		{
			if(OnComplete) OnComplete();
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Error, TEXT("Failed to load SlotMappingAsset."));
		}
	});
}

void UCustomizationComponent::UpdateDebugInfo()
{
	if (!UCustomizationSettings::Get()->GetEnableDebug()) return;

	if (!OwningCharacter.IsValid()) return;
	const FVector CharacterLocation = OwningCharacter->GetMesh()->GetComponentLocation();
	//DrawDebugPoint(GetWorld(), CharacterLocation, 5.f, FColor::White, true, 0.1f, false);

	const FVector PendingItemsLocation = CharacterLocation + FVector(0, 0, DebugInfo.VerticalOffset); // Left
	const FVector SkinCoverageLocation = CharacterLocation + FVector(DebugInfo.HorizontalOffset, 0, DebugInfo.VerticalOffset); // Above
	const FVector EquippedItemsLocation = CharacterLocation + FVector(DebugInfo.HorizontalOffset * 2, 0, DebugInfo.VerticalOffset); // Right
	const FVector SkinInfoLocation = CharacterLocation + FVector(0, 0, DebugInfo.VerticalOffset - DebugInfo.OffsetForSecondLine); // Left, below 
	const FVector ActorInfoLocation = CharacterLocation + FVector(DebugInfo.HorizontalOffset * -2, 0, DebugInfo.VerticalOffset - DebugInfo.OffsetForSecondLine); // Right, below

	DrawDebugTextBlock(PendingItemsLocation, DebugInfo.GetBlockText("Pending Items", DebugInfo.PendingItems), OwningCharacter.Get(), FColor::Turquoise);
	DrawDebugTextBlock(SkinCoverageLocation, DebugInfo.GetBlockText("Skin Coverage", DebugInfo.SkinCoverage), OwningCharacter.Get(), FColor::Green);
	DrawDebugTextBlock(EquippedItemsLocation, DebugInfo.GetBlockText("Equipped Items", DebugInfo.EquippedItems), OwningCharacter.Get(), FColor::Yellow);
	DrawDebugTextBlock(ActorInfoLocation, DebugInfo.GetBlockText("Actor Info", DebugInfo.ActorInfo), OwningCharacter.Get(), FColor::Orange);
	DrawDebugTextBlock(SkinInfoLocation, DebugInfo.GetBlockText("Skin Info", DebugInfo.SkinInfo), OwningCharacter.Get(), FColor::Purple);
}

void UCustomizationComponent::DrawDebugTextBlock(const FVector& Location, const FString& Text, AActor* OwningActor, const FColor& Color)
{
	UKismetSystemLibrary::DrawDebugString(
		GetWorld(),
		Location,
		Text,
		OwningActor,
		Color,
		0.1f
	);
}

void UCustomizationComponent::OnMergeCompleted(USkeletalMesh* MergedMesh)
{
	if (!OwningCharacter.IsValid() || !OwningCharacter->GetMesh())
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("[MESH MERGE] OwningCharacter or its mesh is invalid!"));
		HandleInvalidationPipelineCompleted();
		return;
	}
	if (!MergedMesh)
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("[MESH MERGE] Merge failed, merged mesh is nullptr!"));
		// If merge fails, we might want to clear the mesh to indicate an error state
		OwningCharacter->GetMesh()->SetSkeletalMeshAsset(nullptr);
		HandleInvalidationPipelineCompleted();
		return;
	}
	// Set merged mesh to main component
	OwningCharacter->GetMesh()->SetSkeletalMeshAsset(MergedMesh);
	// Apply materials based on the map filled by the merge subsystem
	USkeletalMeshComponent* TargetMeshComponent = OwningCharacter->GetMesh();
    
	// Now, override with custom skins where applicable
	auto* AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
	if (!AssetManager)
	{
		HandleInvalidationPipelineCompleted();
		return;
	}
    
	for (const auto& MaterialPair : ProcessingTargetState.EquippedMaterialsMap)
	{
		const FGameplayTag& SlotTag = MaterialPair.Key;
		const FName& SkinSlug = MaterialPair.Value;

		for (const auto& MapPair : MergedMaterialMap)
		{
			if (MapPair.Value == SlotTag)
			{
				const int32 MaterialIndex = MapPair.Key;
				FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(SkinSlug);

				if (auto* MaterialAsset = AssetManager->LoadPrimaryAsset<UMaterialCustomizationDataAsset>(SkinAssetId))
				{
					if(MaterialAsset->IndexWithApplyingMaterial.Contains(0))
					{
						UMaterialInterface* SkinMaterial = MaterialAsset->IndexWithApplyingMaterial[0];
						TargetMeshComponent->SetMaterial(MaterialIndex, SkinMaterial);
						UE_LOG(LogCustomizationComponent, Log, TEXT("Applied skin %s to merged mesh at index %d for slot %s"), *SkinSlug.ToString(), MaterialIndex, *SlotTag.ToString());
					}
				}
			}
		}
	}
	ApplyMergedMaterials();
	
	UE_LOG(LogCustomizationComponent, Log, TEXT("[MESH MERGE] Successfully applied merged mesh to main component."));
	HandleInvalidationPipelineCompleted();
}
