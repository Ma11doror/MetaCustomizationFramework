#include "AsyncCustomisation/Public/Components/CustomizationComponent.h"

#include "AsyncCustomisation/Public/BaseCharacter.h"
#include "AsyncCustomisation/Public/Components/Core/CustomizationUtilities.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "AsyncCustomisation/Public/Utilities/CommonUtilities.h"
#include "Components/Core/CustomizationItemBase.h"
#include "Components/Core/Assets/SomatotypeDataAsset.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utilities/CustomizationSettings.h"
#include "Utilities/MetaGameLib.h"


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

	Skeletals = {
		{EBodyPartType::BodySkin, OwningCharacter->GetMesh()},
		{EBodyPartType::Legs, OwningCharacter->LegsMesh},
		{EBodyPartType::Hands, OwningCharacter->HandsMesh},
		{EBodyPartType::Wrists, OwningCharacter->WristsMesh},
		{EBodyPartType::Feet, OwningCharacter->FeetMesh},
		{EBodyPartType::Beard, OwningCharacter->BeardMesh},
		{EBodyPartType::Torso, OwningCharacter->TorsoMesh},
		{EBodyPartType::Neck, OwningCharacter->NeckMesh},

		{EBodyPartType::FaceAccessory, OwningCharacter->FaceAccessoryMesh},
		{EBodyPartType::BackAccessoryFirst, OwningCharacter->BackAccessoryFirstMesh},
		{EBodyPartType::BackAccessorySecondary, OwningCharacter->BackAccessorySecondaryMesh},

		{EBodyPartType::LegKnife, OwningCharacter->LegKnifeMesh},
		{EBodyPartType::Hair, OwningCharacter->HairMesh},
	};

	CurrentCustomizationState.Somatotype = OwningCharacter->Somatotype;
	// TODO:: think about it. Where we get info about somatotype? Maybe: recomend user to create meta data with it by himself

	FCustomizationContextData TargetState = CurrentCustomizationState;
	Invalidate(TargetState, false, ECustomizationInvalidationReason::All);
}

void UCustomizationComponent::EquipItem(const FName& ItemSlug)
{
	if (ItemSlug == NAME_None) return;

	const FPrimaryAssetId ItemCustomizationAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
	if (!ItemCustomizationAssetId.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("EquipItem: Invalid Asset ID for slug %s"), *ItemSlug.ToString());
		return;
	}
	const auto CustomizationAssetClass = CustomizationUtilities::GetClassForCustomizationAsset(ItemCustomizationAssetId);

	// 1. 
	FCustomizationContextData TargetState = CurrentCustomizationState;

	// 2.
	if (!CustomizationAssetClass)
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("EquipItem: Could not determine asset class for %s"), *ItemSlug.ToString());
		return;
	}

	if (CustomizationAssetClass->IsChildOf(UCustomizationDataAsset::StaticClass()))
	{
		const ECustomizationSlotType SlotType = CustomizationSlots::GetSlotTypeForItemBySlug(this, ItemSlug);
		if (OnlyOneItemInSlot)
		{
			//Remove item from slot if needed
			TargetState.EquippedCustomizationItemActors.Remove(SlotType);

			const FEquippedItemActorsInfo EquippedItemActorsInfo = {ItemSlug, {}};
			const FEquippedItemsInSlotInfo EquippedItemsInSlotInfo = {{EquippedItemActorsInfo}};
			TargetState.EquippedCustomizationItemActors.Emplace(SlotType, EquippedItemsInSlotInfo);
		}
		else
		{
			TargetState.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, {});
		}
	}
	else if (CustomizationAssetClass->IsChildOf(UMaterialCustomizationDataAsset::StaticClass()) || CustomizationAssetClass->IsChildOf(UMaterialPackCustomizationDA::StaticClass()))
	{
		// TODO: Handle if something will be wrong with packs of material?
		// 
		TargetState.EquippedMaterialsMap.Add(ItemSlug, EBodyPartType::None); // BodyPart will be known when we load it
	}
	else if (CustomizationAssetClass->IsChildOf(UBodyPartAsset::StaticClass()))
	{
		TargetState.EquippedBodyPartsItems.Add(ItemSlug, EBodyPartType::None);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("EquipItem: Unknown asset class for %s"), *ItemSlug.ToString());
		return;
	}

	// 3.
	Invalidate(TargetState);
}

void UCustomizationComponent::EquipItems(const TArray<FName>& Items)
{
	FCustomizationContextData TargetState = CurrentCustomizationState;
	bool bStateChanged = false;

	for (const FName& ItemSlug : Items)
	{
		if (ItemSlug == NAME_None) continue;

		const FPrimaryAssetId ItemCustomizationAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
		if (!ItemCustomizationAssetId.IsValid()) continue;
		const auto CustomizationAssetClass = CustomizationUtilities::GetClassForCustomizationAsset(ItemCustomizationAssetId);
		if (!CustomizationAssetClass) continue;


		// --- TargetState (EquipItem) ---
		if (CustomizationAssetClass->IsChildOf(UCustomizationDataAsset::StaticClass()))
		{
			const ECustomizationSlotType SlotType = CustomizationSlots::GetSlotTypeForItemBySlug(this, ItemSlug);
			if (OnlyOneItemInSlot)
			{
				TargetState.EquippedCustomizationItemActors.Remove(SlotType);
				const FEquippedItemActorsInfo EquippedItemActorsInfo = {ItemSlug, {}};
				const FEquippedItemsInSlotInfo EquippedItemsInSlotInfo = {{EquippedItemActorsInfo}};
				TargetState.EquippedCustomizationItemActors.Emplace(SlotType, EquippedItemsInSlotInfo);
			}
			else
			{
				TargetState.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, {});
			}
			bStateChanged = true;
		}
		else if (CustomizationAssetClass->IsChildOf(UMaterialCustomizationDataAsset::StaticClass()) || CustomizationAssetClass->IsChildOf(UMaterialPackCustomizationDA::StaticClass()))
		{
			TargetState.EquippedMaterialsMap.Add(ItemSlug, EBodyPartType::None);
			bStateChanged = true;
		}
		else if (CustomizationAssetClass->IsChildOf(UBodyPartAsset::StaticClass()))
		{
			TargetState.EquippedBodyPartsItems.Add(ItemSlug, EBodyPartType::None);
			bStateChanged = true;
		}
	}
	
	if (bStateChanged)
	{
		Invalidate(TargetState);
	}
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
		const ECustomizationSlotType SlotType = CustomizationSlots::GetSlotTypeForItemBySlug(this, ItemSlug);
		if (OnlyOneItemInSlot)
		{
			// Check if this item in slot
			if (const FEquippedItemsInSlotInfo* SlotInfo = TargetState.EquippedCustomizationItemActors.Find(SlotType))
			{
				if (SlotInfo->EquippedItemActors.Num() > 0 && SlotInfo->EquippedItemActors[0].ItemSlug == ItemSlug)
				{
					if (TargetState.EquippedCustomizationItemActors.Remove(SlotType) > 0) bStateChanged = true;
				}
			}
		}
		else if (FEquippedItemsInSlotInfo* EquippedItemsInSlotInfo = TargetState.EquippedCustomizationItemActors.Find(SlotType))
		{
			int32 RemovedCount = EquippedItemsInSlotInfo->EquippedItemActors.RemoveAll(
				[&ItemSlug](const FEquippedItemActorsInfo& EquippedItemActorsInfo) { return EquippedItemActorsInfo.ItemSlug == ItemSlug; });

			if (RemovedCount > 0) bStateChanged = true;

			if (EquippedItemsInSlotInfo->EquippedItemActors.IsEmpty())
			{
				if (TargetState.EquippedCustomizationItemActors.Remove(SlotType) > 0) bStateChanged = true;
			}
		}
	}
	else if (CustomizationAssetClass->IsChildOf(UMaterialCustomizationDataAsset::StaticClass()) || CustomizationAssetClass->IsChildOf(UMaterialPackCustomizationDA::StaticClass()))
	{
		if (TargetState.EquippedMaterialsMap.Remove(ItemSlug) > 0) bStateChanged = true;
	}
	else if (CustomizationAssetClass->IsChildOf(UBodyPartAsset::StaticClass()))
	{
		if (TargetState.EquippedBodyPartsItems.Remove(ItemSlug) > 0) bStateChanged = true;
	}

	// 3.
	if (bStateChanged)
	{
		Invalidate(TargetState);
	}
}

bool UCustomizationComponent::RequestUnequipSlot(ECustomizationSlotType InSlotToUnequip)
{
	//TODO:: 
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
	for (auto& [PartType, Skeletal] : Skeletals)
	{
		if (Skeletal)
		{
			CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, PartType);
		}
	}

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
}

void UCustomizationComponent::SetCustomizationContext(const FCustomizationContextData& InContext)
{
	Invalidate(InContext, false, ECustomizationInvalidationReason::All);
}

const FCustomizationContextData& UCustomizationComponent::GetCurrentCustomizationState()
{
	return CurrentCustomizationState;
}

const TMap<EBodyPartType, USkeletalMeshComponent*>& UCustomizationComponent::GetSkeletals()
{
	return Skeletals;
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
	// --- START OF DEBUG LOGGING ---
	UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: PRE-CheckDiff. CurrentCustomizationState Actors Num: %d. ProcessingTargetState Actors Num: %d"),
	       CurrentCustomizationState.EquippedCustomizationItemActors.Num(),
	       ProcessingTargetState.EquippedCustomizationItemActors.Num()
	);

	bool bCubeOnHandInCurrentStateActors = false;
	FName CubeOnHandSlug(TEXT("cube_on_hand"));

	for (const auto& SlotPair : CurrentCustomizationState.EquippedCustomizationItemActors)
	{
		// SlotPair.Key is ECustomizationSlotType
		// SlotPair.Value is FEquippedItemsInSlotInfo
		for (const FEquippedItemActorsInfo& ItemInfo : SlotPair.Value.EquippedItemActors)
		{
			if (ItemInfo.ItemSlug == CubeOnHandSlug)
			{
				bCubeOnHandInCurrentStateActors = true;
				break;
			}
		}
		if (bCubeOnHandInCurrentStateActors)
		{
			break;
		}
	}

	if (bCubeOnHandInCurrentStateActors)
	{
		UE_LOG(LogCustomizationComponent, Error, TEXT("Invalidate: PRE-CheckDiff: '%s' IS ALREADY IN CurrentCustomizationState.EquippedCustomizationItemActors!"), *CubeOnHandSlug.ToString());
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: PRE-CheckDiff: '%s' IS NOT in CurrentCustomizationState.EquippedCustomizationItemActors. (GOOD)"), *CubeOnHandSlug.ToString());
	}
	// --- END OF DEBUG LOGGING ---


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
	UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: ExplicitReason for this call: %s, CalculatedReasonFromDiff: %s"),
	       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(ExplicitReason)),
	       *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(CalculatedReasonFromDiff))
	);

	ECustomizationInvalidationReason CombinedReason = ExplicitReason;
	EnumAddFlags(CombinedReason, CalculatedReasonFromDiff);
	UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalidate: CombinedReason: %s"), *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(CombinedReason)));


	// 2. Calculate final reason
	// if (CombinedReason == ECustomizationInvalidationReason::None)
	if (static_cast<int64>(CombinedReason) == 0)
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
		MapStr += FString::Printf(TEXT(" || [%s: %s] "), *Pair.Key.ToString(), *UEnum::GetValueAsString(Pair.Value));
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

    if (AssetType.GetName() == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
    {
        UCustomizationAssetManager::StaticAsyncLoadAsset<UMaterialCustomizationDataAsset>(
            SkinMaterialAssetId,
            [this, AssetIdBeingLoaded = SkinMaterialAssetId, SomatotypeAtRequestTime = ForSomatotype](UMaterialCustomizationDataAsset* LoadedMaterialAsset)
            {
                if (this == nullptr || !IsValid(this)) return;

                ESomatotype CurrentActiveSomatotype = ProcessingTargetState == FCustomizationContextData{} ? ProcessingTargetState.Somatotype : CurrentCustomizationState.Somatotype;
                if (SomatotypeAtRequestTime != CurrentActiveSomatotype)
                {
                     UE_LOG(LogCustomizationComponent, Log, TEXT("LoadAndCacheBodySkinMaterial: Somatotype changed while loading %s. Discarding."), *AssetIdBeingLoaded.ToString());
                     return;
                }

                if (LoadedMaterialAsset)
                {
                	// TODO:: hope that skin mesh have only 1 material slot
                    if (LoadedMaterialAsset->BodyPartType == EBodyPartType::BodySkin && LoadedMaterialAsset->IndexWithApplyingMaterial[0])
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
            [this, AssetIdBeingLoaded = SkinMaterialAssetId, SomatotypeAtRequestTime = ForSomatotype](UMaterialPackCustomizationDA* LoadedMaterialPack)
            {
                if (this == nullptr || !IsValid(this)) return;
                
                ESomatotype CurrentActiveSomatotype = ProcessingTargetState == FCustomizationContextData{} ? ProcessingTargetState.Somatotype : CurrentCustomizationState.Somatotype;
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
                        if (MatAsset && MatAsset->BodyPartType == EBodyPartType::BodySkin && MatAsset->IndexWithApplyingMaterial[0])
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
	USkeletalMeshComponent* BodySkinMeshComp = Skeletals.FindRef(EBodyPartType::BodySkin);
	if (BodySkinMeshComp && BodySkinMeshComp->GetSkeletalMeshAsset()) 
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

void UCustomizationComponent::ApplyFallbackMaterialToBodySkinMesh()
{
	USkeletalMeshComponent* BodySkinMeshComp = Skeletals.FindRef(EBodyPartType::BodySkin);
	if (BodySkinMeshComp && BodySkinMeshComp->GetSkeletalMeshAsset())
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
	// 1. 
	TSharedPtr<int32> AssetsToLoadCounter = MakeShared<int32>(0);
	TSharedPtr<bool> bHasPoppedForThisStep = MakeShared<bool>(false);
	auto OnAllSkinsProcessedAndPopCounter = [this, bHasPoppedForThisStep]()
	{
		if (bHasPoppedForThisStep.IsValid() && !(*bHasPoppedForThisStep))
		{
			*bHasPoppedForThisStep = true;
			PendingInvalidationCounter.Pop();
			UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Finished processing all skins. Popped counter."));
		}
		else if (bHasPoppedForThisStep.IsValid() && *bHasPoppedForThisStep)
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateColoration: Attempted to Pop counter again. Already popped this step."));
		}
	};

	auto DecrementCounterAndCheckCompletion = [AssetsToLoadCounter, OnAllSkinsProcessedAndPopCounter]()
	{
		if (AssetsToLoadCounter.IsValid())
		{
			(*AssetsToLoadCounter)--;
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("InvalidateColoration: Asset processed, counter now: %d"), *AssetsToLoadCounter);
			if ((*AssetsToLoadCounter) == 0)
			{
				OnAllSkinsProcessedAndPopCounter();
			}
		}
	};

	auto ApplySkinToTargetMesh = [this](UMaterialCustomizationDataAsset* MaterialCustomizationAsset, USkeletalMeshComponent* TargetMesh)
	{
		if (MaterialCustomizationAsset && TargetMesh)
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("InvalidateColoration: Applying material %s to mesh %s"), *MaterialCustomizationAsset->GetPrimaryAssetId().ToString(), *TargetMesh->GetName());
			CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, TargetMesh);
		}
	};

	auto ProcessSkinAsset = [this, &TargetState, ApplySkinToTargetMesh, AssetsToLoadCounter, DecrementCounterAndCheckCompletion](const FPrimaryAssetId& InSkinAssetId, const FName& InMaterialSlugOptional = NAME_None) mutable
	{
		const FName SkinAssetTypeName = InSkinAssetId.PrimaryAssetType.GetName();
		const FName MaterialSlugToUpdate = (InMaterialSlugOptional == NAME_None) ? InSkinAssetId.PrimaryAssetName : InMaterialSlugOptional;

		UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Processing SkinAsset %s (Slug: %s, Type: %s)"),
		       *InSkinAssetId.ToString(), *MaterialSlugToUpdate.ToString(), *SkinAssetTypeName.ToString());

		if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticAsyncLoadAsset<UMaterialCustomizationDataAsset>(
				InSkinAssetId,
				[this, &TargetState, ApplySkinToTargetMesh, InSkinAssetId, DecrementCounterAndCheckCompletion, MaterialSlugToUpdate](UMaterialCustomizationDataAsset* MaterialCustomizationAsset)
				{
					if (MaterialCustomizationAsset)
					{
						UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Loaded MaterialCustomizationDataAsset %s."), *InSkinAssetId.ToString());
						if (EBodyPartType* TypeInMap = TargetState.EquippedMaterialsMap.Find(MaterialSlugToUpdate))
						{
							if (*TypeInMap != MaterialCustomizationAsset->BodyPartType)
							{
								UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Updating BodyPartType for material %s from %s to %s."),
								       *MaterialSlugToUpdate.ToString(), *UEnum::GetValueAsString(*TypeInMap), *UEnum::GetValueAsString(MaterialCustomizationAsset->BodyPartType));
								*TypeInMap = MaterialCustomizationAsset->BodyPartType;
							}
						}

						if (MaterialCustomizationAsset->bApplyOnBodyPart)
						{
							const auto TargetBodyPartTypeFromAsset = MaterialCustomizationAsset->BodyPartType;
							if (Skeletals.Contains(TargetBodyPartTypeFromAsset) && Skeletals[TargetBodyPartTypeFromAsset])
							{
								ApplySkinToTargetMesh(MaterialCustomizationAsset, Skeletals[TargetBodyPartTypeFromAsset]);
							}
							else
							{
								UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateColoration: SkeletalMesh for BodyPartType %s not found for material %s."), *UEnum::GetValueAsString(TargetBodyPartTypeFromAsset), *MaterialSlugToUpdate.ToString());
							}
						}
					}
					else
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateColoration: Failed to load MaterialCustomizationDataAsset: %s (Slug: %s)"), *InSkinAssetId.ToString(), *MaterialSlugToUpdate.ToString());
					}
					DecrementCounterAndCheckCompletion();
				});
		}
		else if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticAsyncLoadAsset<UMaterialPackCustomizationDA>(
				InSkinAssetId,
				[this, &TargetState, ApplySkinToTargetMesh, InSkinAssetId, DecrementCounterAndCheckCompletion, MaterialSlugToUpdate]
			(UMaterialPackCustomizationDA* MaterialPackAsset)
				{
					if (MaterialPackAsset)
					{
						UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Loaded MaterialPackCustomizationDA %s."), *InSkinAssetId.ToString());
						if (MaterialPackAsset->MaterialAsset.MaterialCustomizations.Num() > 0)
						{
							UMaterialCustomizationDataAsset* FirstItemInPack = MaterialPackAsset->MaterialAsset.MaterialCustomizations[0];
							if (FirstItemInPack)
							{
								EBodyPartType DeducedPackBodyPartType = FirstItemInPack->BodyPartType;
								if (EBodyPartType* TypeInMap = TargetState.EquippedMaterialsMap.Find(MaterialSlugToUpdate))
								{
									if (*TypeInMap != DeducedPackBodyPartType)
									{
										UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Updating BodyPartType for material pack %s from %s to %s (deduced)."),
										       *MaterialSlugToUpdate.ToString(), *UEnum::GetValueAsString(*TypeInMap), *UEnum::GetValueAsString(DeducedPackBodyPartType));
										*TypeInMap = DeducedPackBodyPartType;
									}
								}
							}
						}

						for (UMaterialCustomizationDataAsset* MaterialInPack : MaterialPackAsset->MaterialAsset.MaterialCustomizations)
						{
							if (MaterialInPack && MaterialInPack->bApplyOnBodyPart)
							{
								const auto TargetBodyPartTypeFromAsset = MaterialInPack->BodyPartType;
								if (Skeletals.Contains(TargetBodyPartTypeFromAsset) && Skeletals[TargetBodyPartTypeFromAsset])
								{
									ApplySkinToTargetMesh(MaterialInPack, Skeletals[TargetBodyPartTypeFromAsset]);
								}
								else
								{
									UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateColoration: SkeletalMesh for BodyPartType %s not found for material in pack %s."), *UEnum::GetValueAsString(TargetBodyPartTypeFromAsset), *MaterialSlugToUpdate.ToString());
								}
							}
						}
					}
					else
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateColoration: Failed to load MaterialPackCustomizationDA: %s (Slug: %s)"), *InSkinAssetId.ToString(), *MaterialSlugToUpdate.ToString());
					}
					DecrementCounterAndCheckCompletion();
				});
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Error, TEXT("InvalidateColoration: ProcessSkinAsset called with unknown asset type: %s for AssetId: %s. Decrementing counter."),
			       *SkinAssetTypeName.ToString(), *InSkinAssetId.ToString());
			DecrementCounterAndCheckCompletion();
		}
	};

	// 2. 
	DebugInfo.SkinInfo.Empty();
	int32 InitialOperationsQueued = 0;
	for (const auto& Pair : TargetState.EquippedMaterialsMap)
	{
		FName MaterialSlug = Pair.Key;
		FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(MaterialSlug);
		if (SkinAssetId.IsValid())
		{
			(*AssetsToLoadCounter)++;
			InitialOperationsQueued++;
			DebugInfo.SkinInfo.Append(SkinAssetId.ToString() + TEXT("\n"));
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("InvalidateColoration: Invalid SkinAssetId for explicit slug %s."), *MaterialSlug.ToString());
		}
	}
	UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: Total operations for EXPLICIT materials: %d. Local Counter: %d."), InitialOperationsQueued, *AssetsToLoadCounter);

	if (*AssetsToLoadCounter == 0)
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("InvalidateColoration: No EXPLICIT materials to load. Finalizing skin step."));
		OnAllSkinsProcessedAndPopCounter(); // If no operations, complete the step
		return;
	}

	for (const auto& Pair : TargetState.EquippedMaterialsMap)
	{
		FName MaterialSlug = Pair.Key;
		FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(MaterialSlug);
		if (SkinAssetId.IsValid())
		{
			ProcessSkinAsset(SkinAssetId, MaterialSlug);
		}
	}
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

	// 1. Create a map of ItemSlug to UBodyPartAsset for quick lookups ?
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
	TargetStateToModify.EquippedBodyPartsItems.GetKeys(BodyPartSlugsToResolve);

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
	TSet<EBodyPartType> FinalUsedPartTypes; 
	TArray<FName> FinalActiveSlugs;
	RebuildEquippedBodyPartsState(TargetStateToModify, ResolvedVariantData, FinalUsedPartTypes, FinalActiveSlugs);

	// 6. Apply actual skeletal meshes, the body skin, and reset unused parts
	ApplyBodyPartMeshesAndSkin(
		TargetStateToModify,
		LoadedSomatotypeDataAsset,
		FinalUsedPartTypes,
		FinalActiveSlugs,
		ResolvedVariantData.SlugToResolvedVariantMap
	);

	// 7. 
	DebugInfo.EquippedItems = TargetStateToModify.GetItemsList();
	OnSomatotypeLoaded.Broadcast(LoadedSomatotypeDataAsset);
	UE_LOG(LogCustomizationComponent, Log, TEXT("ProcessBodyParts: Finished. Final TargetStateToModify.EquippedBodyPartsItems.Num: %d"), TargetStateToModify.EquippedBodyPartsItems.Num());
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
                                            TSet<EBodyPartType>& FinalUsedPartTypes,
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
	if (SkinMeshVariant)
	{
		UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying Body Skin Mesh based on flags: %s"), *SkinVisibilityFlags.ToString());
		FinalUsedPartTypes.Emplace(EBodyPartType::BodySkin);
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, SkinMeshVariant->BodyPartSkeletalMesh, EBodyPartType::BodySkin);
		DebugInfo.SkinCoverage = DebugInfo.FormatData(SkinVisibilityFlags);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("No matching Body Skin Mesh found for flags: %s. Resetting skin mesh."), *SkinVisibilityFlags.ToString());
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, EBodyPartType::BodySkin);
		DebugInfo.SkinCoverage = FString::Printf(TEXT("No Match: %s"), *SkinVisibilityFlags.ToString());
	}
}

void UCustomizationComponent::ApplyMaterialToBodyMesh(ESomatotype Somatotype, USkeletalMeshComponent* BodySkinMeshComp)
{
	if (!BodySkinMeshComp || !BodySkinMeshComp->GetSkeletalMeshAsset())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("ApplyDefaultMaterialToBodySkinMesh: Invalid inputs or BodySkin mesh not set."));
		return;
	}

	const FPrimaryAssetId DefaultSkinMaterialAssetId = UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(Somatotype);
	UE_LOG(LogCustomizationComponent, Log, TEXT("ApplyDefaultMaterialToBodySkinMesh: Attempting to apply DefaultSkinMaterialAssetId: %s to %s"),
	       *DefaultSkinMaterialAssetId.ToString(), *BodySkinMeshComp->GetName());

	if (!DefaultSkinMaterialAssetId.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("ApplyDefaultMaterialToBodySkinMesh: No DefaultSkinMaterialAssetId found for Somatotype %s."), *UEnum::GetValueAsString(Somatotype));
		return;
	}

	if (DefaultSkinMaterialAssetId.PrimaryAssetType.GetName() == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
	{
		UCustomizationAssetManager::StaticAsyncLoadAsset<UMaterialCustomizationDataAsset>(
			DefaultSkinMaterialAssetId,
			[WeakThis = MakeWeakObjectPtr(this), BodySkinMeshComp, DefaultSkinMaterialAssetId]
		(UMaterialCustomizationDataAsset* LoadedSkinMaterialAsset)
			{
				if (LoadedSkinMaterialAsset)
				{
					if (LoadedSkinMaterialAsset->BodyPartType == EBodyPartType::BodySkin && LoadedSkinMaterialAsset->bApplyOnBodyPart)
					{
						CustomizationUtilities::SetMaterialOnMesh(LoadedSkinMaterialAsset, BodySkinMeshComp);
						UE_LOG(LogCustomizationComponent, Log, TEXT("ApplyDefaultMaterialToBodySkinMesh: Successfully applied %s."), *DefaultSkinMaterialAssetId.ToString());
					}
					else
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("ApplyDefaultMaterialToBodySkinMesh: Loaded DefaultSkinMaterialAsset %s is not configured for BodySkin or bApplyOnBodyPart is false."), *DefaultSkinMaterialAssetId.ToString());
					}
				}
				else
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("ApplyDefaultMaterialToBodySkinMesh: Failed to load UMaterialCustomizationDataAsset: %s"), *DefaultSkinMaterialAssetId.ToString());
				}
				WeakThis->PendingInvalidationCounter.Pop();
			}
		);
	}
	// else
	// {
	// 	UE_LOG(LogCustomizationComponent, Error, TEXT("ApplyDefaultMaterialToBodySkinMesh: DefaultSkinMaterialAssetId %s has an unsupported PrimaryAssetType: %s"),
	// 	       *DefaultSkinMaterialAssetId.ToString(), *DefaultSkinMaterialAssetId.PrimaryAssetType.ToString());
	// }
}

void UCustomizationComponent::ResetUnusedBodyParts(const FCustomizationContextData& TargetState,
                                                   const TSet<EBodyPartType>& FinalUsedPartTypes)
{
	UE_LOG(LogCustomizationComponent, Verbose, TEXT("Resetting unused body parts (not in FinalUsedPartTypes)..."));
	for (const auto& [PartType, SkeletalComp] : Skeletals)
	{
		if (SkeletalComp && !FinalUsedPartTypes.Contains(PartType))
		{
			if (SkeletalComp->GetSkeletalMeshAsset() != nullptr)
			{
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("Resetting mesh for unused BodyPartType: %s"), *UEnum::GetValueAsString(PartType));
				CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, PartType);
			}
		}
	}
}

void UCustomizationComponent::SpawnAndAttachActorsForItem(UCustomizationDataAsset* DataAsset,
                                                          FName ItemSlug,
                                                          const TMap<EBodyPartType,
                                                          USkeletalMeshComponent*>& CharacterSkeletals,
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
		if (CharacterSkeletals.Contains(EBodyPartType::BodySkin) && CharacterSkeletals.Contains(EBodyPartType::BodySkin) && CharacterSkeletals[EBodyPartType::BodySkin])
		{
			AttachTarget = CharacterSkeletals[EBodyPartType::BodySkin];
		}
		else
		{
			AttachTarget = CharOwner->GetMesh();
			if (AttachTarget)
			{
				UE_LOG(LogCustomizationComponent, Warning, TEXT("SpawnAndAttachActorsForItem: Skeletals map does not contain a valid BodySkin mesh component for item %s. Using OwningCharacter->GetMesh() (%s)."), *ItemSlug.ToString(), *AttachTarget->GetName());
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
		Params.Owner = CharOwner; // OwningCharacter is the owner of the spawned actors?

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
		const FName& Slug = Pair.Key;
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		if (AssetId.IsValid())
		{
			RelevantAssetIdsSet.Add(AssetId);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("CollectRelevantBodyPartAssetIds: Added slug %s (AssetID: %s) from AddedItemsContext."), *Slug.ToString(), *AssetId.ToString());
		}
	}

	// 2. Add assets from removed body parts in the diff
	// These are still relevant as they might affect variant resolution of other items
	// that remain or are newly added.
	for (const auto& Pair : RemovedItemsContext.EquippedBodyPartsItems)
	{
		const FName& Slug = Pair.Key;
		FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		if (AssetId.IsValid())
		{
			RelevantAssetIdsSet.Add(AssetId);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("CollectRelevantBodyPartAssetIds: Added slug %s (AssetID: %s) from RemovedItemsContext."), *Slug.ToString(), *AssetId.ToString());
		}
	}

	// 3. Add assets currently specified in the target state's body part items
	// This ensures all items that are intended to be part of the final state are loaded.
	// This might overlap with AddedItemsContext but TSet handles duplicates.
	for (const auto& Pair : TargetState.EquippedBodyPartsItems)
	{
		const FName& Slug = Pair.Key;
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

void UCustomizationComponent::RebuildEquippedBodyPartsState(FCustomizationContextData& TargetStateToModify, const FResolvedVariantInfo& ResolvedVariantData, TSet<EBodyPartType>& OutFinalUsedPartTypes, TArray<FName>& OutFinalActiveSlugs)
{
	// 1. Clear the current body part assignments in the target state
	TargetStateToModify.EquippedBodyPartsItems.Empty();
	UE_LOG(LogCustomizationComponent, Log, TEXT("RebuildEquippedBodyPartsState: Cleared TargetState.EquippedBodyPartsItems. Rebuilding..."));

	OutFinalUsedPartTypes.Empty();
	OutFinalActiveSlugs.Empty();
	OutFinalActiveSlugs.Reserve(ResolvedVariantData.FinalSlotAssignment.Num());

	// 2. Rebuild based on the final slot assignments from variant resolution
	for (const auto& Pair : ResolvedVariantData.FinalSlotAssignment)
	{
		EBodyPartType BodyPartType = Pair.Key;
		FName Slug = Pair.Value;

		const FBodyPartVariant* const* FoundVariantPtr = ResolvedVariantData.SlugToResolvedVariantMap.Find(Slug);

		if (FoundVariantPtr && (*FoundVariantPtr))
		{
			TargetStateToModify.EquippedBodyPartsItems.Add(Slug, BodyPartType);
			OutFinalUsedPartTypes.Add(BodyPartType);
			OutFinalActiveSlugs.Add(Slug);
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("RebuildEquippedBodyPartsState: Final assignment for BodyPartType %s is Slug %s."), *UEnum::GetValueAsString(BodyPartType), *Slug.ToString());
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("RebuildEquippedBodyPartsState: Variant for winning slug %s (Type %s) not found in SlugToResolvedVariantMap. This should ideally not happen if slug is in FinalSlotAssignment. Skipping rebuild for this item."), *Slug.ToString(), *UEnum::GetValueAsString(BodyPartType));
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

		const FBodyPartVariant* MatchedVariant = BodyPartAsset->GetMatchedVariant(AllEquippedItemAssetIdsInContext);

		if (MatchedVariant && MatchedVariant->IsValid())
		{
			EBodyPartType ResolvedBodyPartType = MatchedVariant->BodyPartType;
			if (ResolvedBodyPartType != EBodyPartType::None)
			{
				// TMap handles conflicts (last one wins for a type)
				Result.FinalSlotAssignment.Add(ResolvedBodyPartType, ItemSlug); 
				Result.SlugToResolvedVariantMap.Add(ItemSlug, MatchedVariant);
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("ResolveBodyPartVariants: Slug '%s' (Asset: %s) resolved to BodyPartType: %s."), *ItemSlug.ToString(), *BodyPartAsset->GetPrimaryAssetId().ToString(), *UEnum::GetValueAsString(ResolvedBodyPartType));
			}
			else
			{
				UE_LOG(LogCustomizationComponent, Warning, TEXT("ResolveBodyPartVariants: Matched variant for slug '%s' has BodyPartType::None. Skipping assignment."), *ItemSlug.ToString());
			}
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("ResolveBodyPartVariants: No valid variant found for slug '%s' using current equipment context. Skipping assignment."), *ItemSlug.ToString());
		}
	}
	return Result;
}

void UCustomizationComponent::ApplyBodyPartMeshesAndSkin(FCustomizationContextData& TargetStateContext, USomatotypeDataAsset* LoadedSomatotypeDataAsset, TSet<EBodyPartType>& FinalUsedPartTypes, const TArray<FName>& FinalActiveSlugs, const TMap<FName, const FBodyPartVariant*>& SlugToResolvedVariantMap)
{
	// 1. Apply the main body skin mesh based on coverage flags
	// FinalUsedPartTypes is passed by reference and might be modified by ApplyBodySkin
	ApplyBodySkin(TargetStateContext, LoadedSomatotypeDataAsset, FinalUsedPartTypes, SlugToResolvedVariantMap);

	// 2. Reset skeletal meshes for any body parts no longer in use (considering potential BodySkin addition)
	ResetUnusedBodyParts(TargetStateContext, FinalUsedPartTypes);

	// 3. Apply skeletal meshes for all active/equipped body parts
	UE_LOG(LogCustomizationComponent, Log, TEXT("ApplyBodyPartMeshesAndSkin: Applying final body part meshes for %d active slugs..."), FinalActiveSlugs.Num());
	for (const FName& Slug : FinalActiveSlugs)
	{
		const EBodyPartType* PartTypePtr = TargetStateContext.EquippedBodyPartsItems.Find(Slug);
		if (!PartTypePtr)
		{
			UE_LOG(LogCustomizationComponent, Error, TEXT("ApplyBodyPartMeshesAndSkin: Slug '%s' not found in rebuilt TargetState.EquippedBodyPartsItems. This is unexpected."), *Slug.ToString());
			continue;
		}
		EBodyPartType PartType = *PartTypePtr;

		const FBodyPartVariant* const* FoundVariantPtr = SlugToResolvedVariantMap.Find(Slug);
		if (FoundVariantPtr && (*FoundVariantPtr) && (*FoundVariantPtr)->BodyPartSkeletalMesh)
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("ApplyBodyPartMeshesAndSkin: Setting mesh for BodyPart: %s (Type: %s) using mesh %s"),
			       *Slug.ToString(),
			       *UEnum::GetValueAsString(PartType),
			       *(*FoundVariantPtr)->BodyPartSkeletalMesh->GetName());
			CustomizationUtilities::SetBodyPartSkeletalMesh(this, (*FoundVariantPtr)->BodyPartSkeletalMesh, PartType);
		}
		else
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("ApplyBodyPartMeshesAndSkin: No valid mesh or variant found for BodyPart: %s (Type: %s). Mesh will not be set. Consider resetting this slot."), *Slug.ToString(), *UEnum::GetValueAsString(PartType));
			// If it's crucial to clear the mesh if no valid one is found:
			// CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, PartType);
		}
	}
}

void UCustomizationComponent::UpdateMaterialsForBodyPartChanges(FCustomizationContextData& TargetStateToModify, const FResolvedVariantInfo& ResolvedVariantData)
{
	// 1. Determine which body part types have changed their assigned item or became unassigned
	TSet<EBodyPartType> AffectedBodyPartTypesForMaterialReset;
	TMap<EBodyPartType, FName> OriginalTypesForInitialSlugs;

	for (const FName& OldSlug : ResolvedVariantData.InitialSlugsInTargetState)
	{
		const FBodyPartVariant* const* OldVariantPtr = ResolvedVariantData.SlugToResolvedVariantMap.Find(OldSlug);
		if (OldVariantPtr && *OldVariantPtr)
		{
			EBodyPartType OldType = (*OldVariantPtr)->BodyPartType;
			if (OldType != EBodyPartType::None)
			{
				OriginalTypesForInitialSlugs.Add(OldType, OldSlug);

				const FName* WinningSlugInSlot = ResolvedVariantData.FinalSlotAssignment.Find(OldType);
				if (!WinningSlugInSlot || *WinningSlugInSlot != OldSlug)
				{
					AffectedBodyPartTypesForMaterialReset.Add(OldType);
				}
			}
			// If OldType was None, it didn't occupy a slot, so no change to track here for *this* slug.
		}
		else // This slug did not resolve to a valid variant (e.g., item removed or context changed)
		{
			// If this slug *previously* occupied a slot in the *original* TargetState (before ProcessBodyParts),
			// and that slot is now empty or taken by another item, that slot needs to be considered.
			// This is implicitly handled: if a slot in FinalSlotAssignment is different from what it was,
			// or if a previously occupied slot is now empty in FinalSlotAssignment, it will be affected.
			// We iterate over InitialSlugsInTargetState to find what *was* there.
			// If a slug from InitialSlugsInTargetState is NOT in SlugToResolvedVariantMap with a valid type,
			// any EBodyPartType it might have previously held is now 'vacant' by this slug.
			// The logic below for FinalSlotAssignment handles new assignments.
			// The current loop handles slots that *were* occupied by items in InitialSlugsInTargetState.
		}
	}

	// 2. Check for newly assigned body part types
	for (const auto& FinalPair : ResolvedVariantData.FinalSlotAssignment)
	{
		EBodyPartType FinalType = FinalPair.Key;
		FName AssignedSlug = FinalPair.Value;
		if (FinalType != EBodyPartType::None)
		{
			const FName* OriginalSlugInSlot = OriginalTypesForInitialSlugs.Find(FinalType);
			if (!OriginalSlugInSlot || *OriginalSlugInSlot != AssignedSlug)
			{
				// This slot either was empty, or its item changed.
				AffectedBodyPartTypesForMaterialReset.Add(FinalType);
			}
		}
	}

	if (AffectedBodyPartTypesForMaterialReset.Num() > 0)
	{
		FString AffectedTypesStr;
		for (EBodyPartType Type : AffectedBodyPartTypesForMaterialReset) AffectedTypesStr += UEnum::GetValueAsString(Type) + TEXT(" ");
		UE_LOG(LogCustomizationComponent, Log, TEXT("UpdateMaterialsForBodyPartChanges: BodyPart slots potentially affected material-wise: [%s]. Clearing associated materials."), *AffectedTypesStr.TrimEnd());

		TArray<FName> MaterialSlugsToRemove;
		MaterialSlugsToRemove.Reserve(TargetStateToModify.EquippedMaterialsMap.Num());

		for (const auto& MaterialPair : TargetStateToModify.EquippedMaterialsMap)
		{
			if (MaterialPair.Value != EBodyPartType::None && AffectedBodyPartTypesForMaterialReset.Contains(MaterialPair.Value))
			{
				MaterialSlugsToRemove.Add(MaterialPair.Key);
			}
		}

		if (MaterialSlugsToRemove.Num() > 0)
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("UpdateMaterialsForBodyPartChanges: Removing %d materials from EquippedMaterialsMap."), MaterialSlugsToRemove.Num());
			for (const FName& SlugToRemove : MaterialSlugsToRemove)
			{
				TargetStateToModify.EquippedMaterialsMap.Remove(SlugToRemove);
				UE_LOG(LogCustomizationComponent, Log, TEXT("UpdateMaterialsForBodyPartChanges: Removed material %s from EquippedMaterialsMap."), *SlugToRemove.ToString());
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
					Skeletals,
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

FAttachedActorChanges UCustomizationComponent::DetermineAttachedActorChanges(const FCustomizationContextData& CurrentState, const FCustomizationContextData& TargetState)
{
	FAttachedActorChanges Changes;

	// 1. Determine actors to destroy
	for (const auto& Pair : CurrentState.EquippedCustomizationItemActors)
	{
		const ECustomizationSlotType SlotType = Pair.Key;
		const FEquippedItemsInSlotInfo& ItemsInSlot = Pair.Value;

		for (const FEquippedItemActorsInfo& CurrentActorInfo : ItemsInSlot.EquippedItemActors)
		{
			const FName& CurrentSlug = CurrentActorInfo.ItemSlug;
			UE_LOG(LogCustomizationComponent, Warning, TEXT("DetermineAttachedActorChanges: TargetState has Actor Slug: %s"), *CurrentSlug.ToString());
			bool bFoundInTarget = false;
			if (const FEquippedItemsInSlotInfo* TargetSlotInfo = TargetState.EquippedCustomizationItemActors.Find(SlotType))
			{
				if (TargetSlotInfo->EquippedItemActors.ContainsByPredicate(
					[&CurrentSlug](const FEquippedItemActorsInfo& TargetActorInfo) { return TargetActorInfo.ItemSlug == CurrentSlug; }))
				{
					bFoundInTarget = true;
				}
			}

			if (!bFoundInTarget)
			{
				Changes.ActorsToDestroy.Add(CurrentSlug, CurrentActorInfo.ItemRelatedActors); 
			}
		}
	}

	// 2. Determine new assets to load for spawning
	for (const auto& Pair : TargetState.EquippedCustomizationItemActors)
	{
		const ECustomizationSlotType SlotType = Pair.Key;
		const FEquippedItemsInSlotInfo& TargetItemsInSlot = Pair.Value;

		for (const FEquippedItemActorsInfo& TargetActorInfo : TargetItemsInSlot.EquippedItemActors)
		{
			const FName& TargetSlug = TargetActorInfo.ItemSlug;

			bool bFoundInCurrent = false;
			if (const FEquippedItemsInSlotInfo* CurrentSlotInfo = CurrentState.EquippedCustomizationItemActors.Find(SlotType))
			{
				if (CurrentSlotInfo->EquippedItemActors.ContainsByPredicate(
					[&TargetSlug](const FEquippedItemActorsInfo& CurrentActorInfo) { return CurrentActorInfo.ItemSlug == TargetSlug; }))
				{
					bFoundInCurrent = true;
				}
			}

			UE_LOG(LogCustomizationComponent, Error, TEXT("DEBUG: For TargetSlug '%s': bFoundInCurrent = %d"), *TargetSlug.ToString(), bFoundInCurrent);
			FPrimaryAssetId AssetIdForDebug = CommonUtilities::ItemSlugToCustomizationAssetId(TargetSlug);
			UE_LOG(LogCustomizationComponent, Error, TEXT("DEBUG: For TargetSlug '%s': AssetId = [%s], IsValid = %d, Type = [%s], Name = [%s]"),
			       *TargetSlug.ToString(),
			       *AssetIdForDebug.ToString(),
			       AssetIdForDebug.IsValid(),
			       *AssetIdForDebug.PrimaryAssetType.ToString(),
			       *AssetIdForDebug.PrimaryAssetName.ToString());

			if (!bFoundInCurrent)
			{
				FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(TargetSlug);
				if (AssetId.IsValid())
				{
					Changes.AssetIdsToLoad.AddUnique(AssetId);
					Changes.AssetIdToSlugMapForLoad.Add(AssetId, TargetSlug);
					Changes.SlugToSlotMapForLoad.Add(TargetSlug, SlotType);
				}
				else
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("DetermineAttachedActorChanges: Invalid AssetId for ItemSlug: %s. Cannot load for spawning."), *TargetSlug.ToString());
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
