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
		{EBodyPartType::Torso, OwningCharacter->GoatMesh},
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

    // 
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

    // 1. Copy Current state
    FCustomizationContextData TargetState = CurrentCustomizationState;
    bool bStateChanged = false;

	// 2. Modify TargetState
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
                    if(TargetState.EquippedCustomizationItemActors.Remove(SlotType) > 0) bStateChanged = true;
                }
            }
		}
		else if (FEquippedItemsInSlotInfo* EquippedItemsInSlotInfo = TargetState.EquippedCustomizationItemActors.Find(SlotType))
		{
			int32 RemovedCount = EquippedItemsInSlotInfo->EquippedItemActors.RemoveAll(
				[&ItemSlug](const FEquippedItemActorsInfo& EquippedItemActorsInfo){ return EquippedItemActorsInfo.ItemSlug == ItemSlug; });

            if (RemovedCount > 0) bStateChanged = true;
			
            if (EquippedItemsInSlotInfo->EquippedItemActors.IsEmpty())
            {
                if(TargetState.EquippedCustomizationItemActors.Remove(SlotType) > 0) bStateChanged = true;
            }
		}
	}
	else if (CustomizationAssetClass->IsChildOf(UMaterialCustomizationDataAsset::StaticClass()) || CustomizationAssetClass->IsChildOf(UMaterialPackCustomizationDA::StaticClass()))
	{
		if(TargetState.EquippedMaterialsMap.Remove(ItemSlug) > 0) bStateChanged = true;
	}
	else if (CustomizationAssetClass->IsChildOf(UBodyPartAsset::StaticClass()))
	{
		if(TargetState.EquippedBodyPartsItems.Remove(ItemSlug) > 0) bStateChanged = true;
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
		if(Skeletal)
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
					const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
					ItemRelatedActor->DetachFromActor(DetachmentTransformRules);
					ItemRelatedActor->Destroy();
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
		if (DeferredTargetState.IsSet() && DeferredTargetState.GetValue() == TargetState) return;

		EnumAddFlags(DefferReason, ExplicitReason);
		DeferredTargetState = TargetState;
		StartInvalidationTimer(TargetState);
		return;
	}
	FCustomizationContextData ModifiableTargetState = TargetState;

	UE_LOG(LogCustomizationComponent, Log, TEXT("Starting Immediate Invalidation..."));

	// 1. Get diff (CurrentCustomizationState) and (TargetState)
	InvalidationContext.Current = CurrentCustomizationState;
	InvalidationContext.CheckDiff(ModifiableTargetState);

	// 2. Calculate final reason
	ECustomizationInvalidationReason Reason = ExplicitReason;
	EnumAddFlags(Reason, InvalidationContext.CalculateReason());

	if (Reason == ECustomizationInvalidationReason::None)
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidation skipped: No reason found."));
		if (CurrentCustomizationState != TargetState)
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("Updating CurrentCustomizationState to TargetState without full invalidation as Reason is None."));
			CurrentCustomizationState = TargetState;
		}
		InvalidationContext.ClearTemporaryContext();
		DeferredTargetState.Reset();
		return;
	}

	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidation Reason: %s"), *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(Reason)));

	// 3. Execute invalidation steps synchronously
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Body))
	{
		PendingInvalidationCounter.Push();
		InvalidateBodyParts(ModifiableTargetState);
		PendingInvalidationCounter.Pop();
		// TODO: When InvalidateBodyParts becomes async, move this Pop() into its final completion callback.
	}
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Actors))
	{
		PendingInvalidationCounter.Push();
		InvalidateAttachedActors(ModifiableTargetState);
		PendingInvalidationCounter.Pop();
		// TODO: When InvalidateAttachedActors becomes async, move this Pop() into its final completion callback.
	}
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Skin))
	{
		PendingInvalidationCounter.Push(); 
		InvalidateSkin(ModifiableTargetState);
		PendingInvalidationCounter.Pop();
		// TODO: When InvalidateSkin becomes async, move this Pop() into its final completion callback.
	}

	// Log final state of body parts after potential modifications
	FString MapStr;
	for (const auto& Pair : ModifiableTargetState.EquippedBodyPartsItems)
	{
		MapStr += FString::Printf(TEXT(" || [%s: %s] "), *Pair.Key.ToString(), *UEnum::GetValueAsString(Pair.Value));
	}
	UE_LOG(LogCustomizationComponent, Log, TEXT("Target Body parts applied (final): %s"), *MapStr);

	// 4. update current to the potentially modified target state after all invalidates
	CurrentCustomizationState = ModifiableTargetState;
	
	OnEquippedItemsChanged.Broadcast(CurrentCustomizationState);
	
	InvalidationContext.ClearTemporaryContext();
	DeferredTargetState.Reset();

	UE_LOG(LogCustomizationComponent, Log, TEXT("Immediate Invalidation Finished. Current state updated."));
}

void UCustomizationComponent::OnDefferInvalidationTimerExpired()
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("Deferred invalidation timer expired."));
	if (DeferredTargetState.IsSet())
	{
		Invalidate(DeferredTargetState.GetValue(), false, DefferReason);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("Deferred invalidation timer expired, but no TargetState was stored."));
	}
	DefferReason = ECustomizationInvalidationReason::None;
}

void UCustomizationComponent::InvalidateSkin(FCustomizationContextData& TargetState)
{
	 UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidating Skin based on TargetState..."));

	auto ApplySkinToTarget = [this](UMaterialCustomizationDataAsset* MaterialCustomizationAsset, USkeletalMeshComponent* TargetMesh)
	{
		if (MaterialCustomizationAsset && TargetMesh)
		{
			UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying material %s to mesh %s"), *MaterialCustomizationAsset->GetPrimaryAssetId().ToString(), *TargetMesh->GetName());
			CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, TargetMesh);
		}
	};

	auto ApplySkin = [this, ApplySkinToTarget](const FPrimaryAssetId& InSkinAssetId) mutable // Removed counter capture&usage
	{
		const FName SkinAssetTypeName = InSkinAssetId.PrimaryAssetType.GetName();
		if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialCustomizationDataAsset>(
				InSkinAssetId, [this, ApplySkinToTarget, InSkinAssetId](UMaterialCustomizationDataAsset* MaterialCustomizationAsset) // Removed counter capture
				{
					if (MaterialCustomizationAsset && MaterialCustomizationAsset->bApplyOnBodyPart)
					{
						const auto TargetBodyPartType = MaterialCustomizationAsset->BodyPartType;
						if (Skeletals.Contains(TargetBodyPartType))
						{
							if(USkeletalMeshComponent* TargetBodyPart = Skeletals[TargetBodyPartType])
							{
								ApplySkinToTarget(MaterialCustomizationAsset, TargetBodyPart);
							}
						}
					}
					else if (!MaterialCustomizationAsset)
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("Failed to load MaterialCustomizationDataAsset: %s"), *InSkinAssetId.ToString());
					}
				});
		}
		else if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialPackCustomizationDA>(
				InSkinAssetId, [this, ApplySkinToTarget, InSkinAssetId](UMaterialPackCustomizationDA* InSkinAsset) // Removed counter capture
				{
					if (InSkinAsset)
					{
						const auto& AssetCollection = InSkinAsset->MaterialAsset;
						for (const auto& MaterialCustomizationAsset : AssetCollection.MaterialCustomizations)
						{
							if (MaterialCustomizationAsset && MaterialCustomizationAsset->bApplyOnBodyPart)
							{
								const auto TargetBodyPartType = MaterialCustomizationAsset->BodyPartType;
								if (Skeletals.Contains(TargetBodyPartType))
								{
									if(USkeletalMeshComponent* TargetBodyPart = Skeletals[TargetBodyPartType])
									{
										ApplySkinToTarget(MaterialCustomizationAsset, TargetBodyPart);
									}
								}
							}
						}
					}
					else
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("Failed to load MaterialPackCustomizationDA: %s"), *InSkinAssetId.ToString());
					}
				});
		}
	};

	// --- InvalidateSkin ---
	// Step 0: Apply default skin (TargetState)
	const FPrimaryAssetId DefaultSkin = UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(TargetState.Somatotype);
	if (DefaultSkin.IsValid())
	{
		ApplySkin(DefaultSkin);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("No Default Skin found for Somatotype: %s"), *UEnum::GetValueAsString(TargetState.Somatotype));
		// Reset materials to default if no skin asset is found
		for(auto const& [BodyPartType, SkeletalComp] : Skeletals)
		{
			if (SkeletalComp && SkeletalComp->GetSkeletalMeshAsset())
			{
				int32 NumMaterials = SkeletalComp->GetMaterials().Num();
				const auto& DefaultMaterials = SkeletalComp->GetSkeletalMeshAsset()->GetMaterials();
				for (int32 i = 0; i < NumMaterials; ++i)
				{
					 if (DefaultMaterials.IsValidIndex(i))
					 {
						 SkeletalComp->SetMaterial(i, DefaultMaterials[i].MaterialInterface);
					 }
				}
			}
		}
	}

	DebugInfo.SkinInfo = FString{};

	// Apply equipped skins (TargetState)
	for (const auto& [MaterialSlug, BodyPartType] : TargetState.EquippedMaterialsMap)
	{
		FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(MaterialSlug);
		if (SkinAssetId.IsValid())
		{
			ApplySkin(SkinAssetId);
			DebugInfo.SkinInfo.Append(SkinAssetId.ToString() + "\n");
		}
	}
	UE_LOG(LogCustomizationComponent, Log, TEXT("Finished Invalidating Skin."));
}

void UCustomizationComponent::InvalidateBodyParts(FCustomizationContextData& TargetState)
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidating Body Parts based on TargetState..."));

	const auto AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
	if (!AssetManager) {
		UE_LOG(LogCustomizationComponent, Error, TEXT("AssetManager is null!"));
		return;
	}

	const FPrimaryAssetId SomatotypeAssetId = CustomizationUtilities::GetSomatotypeAssetId(TargetState.Somatotype);
	if (!SomatotypeAssetId.IsValid())
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalid Somatotype AssetId!"));
		return;
	}

	AssetManager->SyncLoadAsset<USomatotypeDataAsset>(SomatotypeAssetId, [this, &TargetState](USomatotypeDataAsset* SomatotypeDataAsset) 
	{
		if (!SomatotypeDataAsset)
		{
			UE_LOG(LogCustomizationComponent, Warning, TEXT("Failed to load SomatotypeDataAsset!"));
			return;
		}
		TSet<FPrimaryAssetId> RelevantAssetIdsSet;
		UE_LOG(LogCustomizationComponent, Log, TEXT("Building RelevantAssetIdsSet..."));

		// 1. From Added
		UE_LOG(LogCustomizationComponent, Log, TEXT("Processing Added Items:"));
		for (const auto& [Slug, PartType] : InvalidationContext.Added.EquippedBodyPartsItems)
		{
			FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
			UE_LOG(LogCustomizationComponent, Log, TEXT("  Slug: %s -> AssetId: %s"), *Slug.ToString(), *AssetId.ToString());
			RelevantAssetIdsSet.Add(AssetId);
		}

		// 2. From Removed
		UE_LOG(LogCustomizationComponent, Log, TEXT("Processing Removed Items:"));
		for (const auto& [Slug, PartType] : InvalidationContext.Removed.EquippedBodyPartsItems)
		{
			FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
			UE_LOG(LogCustomizationComponent, Log, TEXT("  Slug: %s -> AssetId: %s"), *Slug.ToString(), *AssetId.ToString());
			RelevantAssetIdsSet.Add(AssetId);
		}

		// 3. From Target State
		UE_LOG(LogCustomizationComponent, Log, TEXT("Processing Target State Items:"));
		for (const auto& [Slug, PartType] : TargetState.EquippedBodyPartsItems)
		{
			FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
			UE_LOG(LogCustomizationComponent, Log, TEXT("  Slug: %s -> AssetId: %s"), *Slug.ToString(), *AssetId.ToString());
			RelevantAssetIdsSet.Add(AssetId);
		}
		
		TArray<FPrimaryAssetId> AllRelevantItemAssetIds = RelevantAssetIdsSet.Array();
		UE_LOG(LogCustomizationComponent, Log, TEXT("Final AllRelevantItemAssetIds size: %d"), AllRelevantItemAssetIds.Num());

		// Load and process body parts synchronously
		LoadAndProcessBodyParts(TargetState, SomatotypeDataAsset, AllRelevantItemAssetIds);
	});

	UE_LOG(LogCustomizationComponent, Log, TEXT("Finished Invalidating Body Parts."));
}

void UCustomizationComponent::LoadAndProcessBodyParts(FCustomizationContextData& TargetState,
													  USomatotypeDataAsset* SomatotypeDataAsset,
													  TArray<FPrimaryAssetId>& AllRelevantItemAssetIds)
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("Loading and Processing %d Body Part Assets for replacement logic..."), AllRelevantItemAssetIds.Num());

	// Since loading is sync, the lambda executes immediately 
	// No Push() here
	UCustomizationAssetManager::StaticSyncLoadAssetList<UBodyPartAsset>(AllRelevantItemAssetIds,
		[this, &TargetState, SomatotypeDataAsset] // Removed counter capture
		(TArray<UBodyPartAsset*> LoadedBodyParts)
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("Loaded %d Body Part Assets."), LoadedBodyParts.Num());

			if (!SomatotypeDataAsset) {
				UE_LOG(LogCustomizationComponent, Error, TEXT("SomatotypeDataAsset is null during processing!"));
				return;
			}

			// --- Logic for determining final body parts (unchanged from original, just ensure no counter calls) ---
			TMap<FName, UBodyPartAsset*> SlugToAssetMap;
			for (auto* BodyPartAsset : LoadedBodyParts) {
				if (BodyPartAsset) SlugToAssetMap.Add(BodyPartAsset->GetPrimaryAssetId().PrimaryAssetName, BodyPartAsset);
			}

			TMap<EBodyPartType, FName> FinalSlotAssignment;
			TMap<FName, const FBodyPartVariant*> AllValidVariants;

			TArray<FName> ExplicitlyRequestedSlugs;
			TargetState.EquippedBodyPartsItems.GenerateKeyArray(ExplicitlyRequestedSlugs);

			// Get asset IDs intended for the final state to resolve variants correctly
			const TArray<FPrimaryAssetId> IntendedEquipmentAssetIds = CommonUtilities::ItemSlugsToAssetIds(TargetState.GetEquippedSlugs());

			auto GetVariantInfo = [&](const FName& Slug, EBodyPartType& OutType, const FBodyPartVariant*& OutVariant) -> bool
			{
				if (const FBodyPartVariant** CachedVariant = AllValidVariants.Find(Slug))
				{
					OutVariant = *CachedVariant;
				}
				// If not cached, find asset and resolve variant
				else if (UBodyPartAsset** FoundAssetPtr = SlugToAssetMap.Find(Slug))
				{
					UBodyPartAsset* Asset = *FoundAssetPtr;
					if (!Asset) return false;

					OutVariant = Asset->GetMatchedVariant(IntendedEquipmentAssetIds);
					if (OutVariant && OutVariant->IsValid())
					{
						AllValidVariants.Add(Slug, OutVariant);
					}
					else
					{
						OutVariant = nullptr;
					}
				}
				else
				{
					OutVariant = nullptr;
				}

				if (OutVariant)
				{
					OutType = OutVariant->BodyPartType;
					return true;
				}
				else
				{
					OutType = EBodyPartType::None;
					return false;
				}
			};

			// Assign explicitly requested items first
			for (const FName& Slug : ExplicitlyRequestedSlugs)
			{
				EBodyPartType ItemBodyPartType = EBodyPartType::None;
				const FBodyPartVariant* ItemVariant = nullptr;

				if (GetVariantInfo(Slug, ItemBodyPartType, ItemVariant))
				{
					if (ItemBodyPartType != EBodyPartType::None)
					{
						FinalSlotAssignment.Add(ItemBodyPartType, Slug);
						UE_LOG(LogCustomizationComponent, Verbose, TEXT("Assigning BodyPartType %s to explicit item '%s' (replacing previous if any)."),
						       *UEnum::GetValueAsString(ItemBodyPartType), *Slug.ToString());
					}
					else
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("Explicit item '%s' resolved to BodyPartType::None. Skipping assignment."), *Slug.ToString());
					}
				}
				else
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("No valid variant found for explicit item '%s'. Skipping assignment."), *Slug.ToString());
				}
			}

			// Rebuild TargetState based on final assignments
			TargetState.EquippedBodyPartsItems.Empty();
			TMap<FName, const FBodyPartVariant*> FinalSlugToVariantMap;
			TSet<EBodyPartType> FinalUsedPartTypes;
			TArray<FName> FinalActiveSlugs;

			UE_LOG(LogCustomizationComponent, Log, TEXT("Rebuilding TargetState based on final slot assignments..."));
			for (const auto& Pair : FinalSlotAssignment)
			{
				EBodyPartType BodyPartType = Pair.Key;
				FName Slug = Pair.Value;

				if (const FBodyPartVariant* const* FoundVariant = AllValidVariants.Find(Slug))
				{
					TargetState.EquippedBodyPartsItems.Add(Slug, BodyPartType); // Update TargetState
					FinalSlugToVariantMap.Add(Slug, *FoundVariant);
					FinalUsedPartTypes.Add(BodyPartType);
					FinalActiveSlugs.Add(Slug);
					UE_LOG(LogCustomizationComponent, Verbose, TEXT("Final state includes: Slug='%s', Type='%s'"), *Slug.ToString(), *UEnum::GetValueAsString(BodyPartType));
				}
				else
				{
					UE_LOG(LogCustomizationComponent, Error, TEXT("Could not find cached variant for final item '%s'. State might be incorrect."), *Slug.ToString());
				}
			}

			// Apply meshes and reset unused parts based on the final calculated state
			ApplyBodySkin(TargetState, SomatotypeDataAsset, FinalUsedPartTypes, FinalSlugToVariantMap);

			UE_LOG(LogCustomizationComponent, Log, TEXT("Applying final body part meshes..."));
			for (const FName& Slug : FinalActiveSlugs)
			{
				if (const FBodyPartVariant* const* FoundVariant = FinalSlugToVariantMap.Find(Slug))
				{
					EBodyPartType PartType = TargetState.EquippedBodyPartsItems.FindChecked(Slug); // Get type from updated TargetState
					UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying mesh for BodyPart: %s (Type: %s)"), *Slug.ToString(), *UEnum::GetValueAsString(PartType));
					CustomizationUtilities::SetBodyPartSkeletalMesh(this, (*FoundVariant)->BodyPartSkeletalMesh, PartType);
				}
			}

			ResetUnusedBodyParts(TargetState, FinalUsedPartTypes);

			DebugInfo.EquippedItems = TargetState.GetItemsList();

			// Note: InvalidateSkin is called by the main Invalidate function if needed.
			// Calling it here again might be redundant depending on the main flow. Removed explicit call.
			// InvalidateSkin(TargetState);

			OnSomatotypeLoaded.Broadcast(SomatotypeDataAsset);
		});
	
	UE_LOG(LogCustomizationComponent, Log, TEXT("Exiting LoadAndProcessBodyParts (sync load finished)."));
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
		if(Pair.Value)
		{
			SkinVisibilityFlags.AddFlag(Pair.Value->SkinCoverageFlags.FlagMask);
		}
	}

	auto SkinMeshVariant = SkinVisibilityFlags.GetMatch(SomatotypeDataAsset->SkinAssociation, SkinVisibilityFlags.FlagMask);
	if (SkinMeshVariant)
	{
		UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying Body Skin Mesh based on flags: %s"), *SkinVisibilityFlags.ToString());
		FinalUsedPartTypes.Emplace(EBodyPartType::BodySkin); // Add BodySkin to the set of used parts
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, SkinMeshVariant->BodyPartSkeletalMesh, EBodyPartType::BodySkin);
		DebugInfo.SkinCoverage = DebugInfo.FormatData(SkinVisibilityFlags);
	}
	else
	{
		UE_LOG(LogCustomizationComponent, Warning, TEXT("No matching Body Skin Mesh found for flags: %s. Resetting skin mesh."), *SkinVisibilityFlags.ToString());
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, EBodyPartType::BodySkin); // Reset skin mesh
		DebugInfo.SkinCoverage = FString::Printf(TEXT("No Match: %s"), *SkinVisibilityFlags.ToString());
	}
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

void UCustomizationComponent::InvalidateAttachedActors(FCustomizationContextData& TargetState)
{
	 UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidating Attached Actors based on TargetState..."));

	// --- Calculate actors to remove based on diff ---
	TMap<ECustomizationSlotType, TArray<FName>> SlugsToRemovePerSlot;
	// Compare Current (InvalidationContext.Current holds the state *before* this invalidation run) vs TargetState
	for(const auto& CurrentPair : InvalidationContext.Current.EquippedCustomizationItemActors) // Use context's 'current'
	{
		ECustomizationSlotType CurrentSlot = CurrentPair.Key;
		const FEquippedItemsInSlotInfo& CurrentSlotInfo = CurrentPair.Value;
		const FEquippedItemsInSlotInfo* TargetSlotInfoPtr = TargetState.EquippedCustomizationItemActors.Find(CurrentSlot);

		if (TargetSlotInfoPtr) // Slot exists in target
		{
			for(const auto& CurrentActorInfo : CurrentSlotInfo.EquippedItemActors)
			{
				bool bFoundInTarget = TargetSlotInfoPtr->EquippedItemActors.ContainsByPredicate(
					[&](const FEquippedItemActorsInfo& TargetActorInfo){ return TargetActorInfo.ItemSlug == CurrentActorInfo.ItemSlug; }
				);
				if (!bFoundInTarget)
				{
					SlugsToRemovePerSlot.FindOrAdd(CurrentSlot).Add(CurrentActorInfo.ItemSlug);
				}
			}
		}
		else // Slot doesn't exist in target, remove all items from it
		{
			 for(const auto& CurrentActorInfo : CurrentSlotInfo.EquippedItemActors)
			 {
				 SlugsToRemovePerSlot.FindOrAdd(CurrentSlot).Add(CurrentActorInfo.ItemSlug);
			 }
		}
	}

	// --- 1: Remove useless actors (based on diff calculated above) ---
	for(const auto& RemovePair : SlugsToRemovePerSlot)
	{
		ECustomizationSlotType SlotToRemove = RemovePair.Key;
		const TArray<FName>& SlugsToRemove = RemovePair.Value;

		// Find the actors to remove in the *actual* current state (before update) stored in context
		if(const FEquippedItemsInSlotInfo* CurrentSlotInfoPtr = InvalidationContext.Current.EquippedCustomizationItemActors.Find(SlotToRemove))
		{
			for(const FName& Slug : SlugsToRemove)
			{
				// Find the specific item info in the current state context
				const FEquippedItemActorsInfo* ActorInfoPtr = CurrentSlotInfoPtr->EquippedItemActors.FindByPredicate(
					[&](const FEquippedItemActorsInfo& Info){ return Info.ItemSlug == Slug; });

				if (ActorInfoPtr)
				{
					UE_LOG(LogCustomizationComponent, Verbose, TEXT("Removing actors for ItemSlug: %s in Slot: %s"), *Slug.ToString(), *UEnum::GetValueAsString(SlotToRemove));
					for (const auto& ItemRelatedActor : ActorInfoPtr->ItemRelatedActors) // Use actors from context
					{
						if (ItemRelatedActor.IsValid())
						{
							const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
							ItemRelatedActor->DetachFromActor(DetachmentTransformRules);
							ItemRelatedActor->Destroy();
						}
					}
				}
			}
		}
	}

	// --- 2: Target&Current: Get slugs for update (items present in Target but not in Current) ---
	TArray<FPrimaryAssetId> AddedOrUpdatedAssetIds;
	TMap<FPrimaryAssetId, FName> AssetIdToSlugMap;
	TMap<FName, ECustomizationSlotType> SlugToSlotMap;

	for(const auto& TargetPair : TargetState.EquippedCustomizationItemActors)
	{
		ECustomizationSlotType TargetSlot = TargetPair.Key;
		const FEquippedItemsInSlotInfo& TargetSlotInfo = TargetPair.Value;
		const FEquippedItemsInSlotInfo* CurrentSlotInfoPtr = InvalidationContext.Current.EquippedCustomizationItemActors.Find(TargetSlot); // Compare against context's current

		for(const auto& TargetActorInfo : TargetSlotInfo.EquippedItemActors)
		{
			FName TargetSlug = TargetActorInfo.ItemSlug;
			bool bFoundInCurrent = false;
			if (CurrentSlotInfoPtr)
			{
				 bFoundInCurrent = CurrentSlotInfoPtr->EquippedItemActors.ContainsByPredicate(
					 [&](const FEquippedItemActorsInfo& CurrentActorInfo){ return CurrentActorInfo.ItemSlug == TargetSlug; }
				 );
			}

			if (!bFoundInCurrent) // If not found in current state context, it needs to be added
			{
				 FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(TargetSlug);
				 if (AssetId.IsValid())
				 {
					 AddedOrUpdatedAssetIds.AddUnique(AssetId);
					 AssetIdToSlugMap.Add(AssetId, TargetSlug);
					 SlugToSlotMap.Add(TargetSlug, TargetSlot);
				 }
			}
		}
	}

	// --- 3: Load and Spawn ---
	if (AddedOrUpdatedAssetIds.IsEmpty())
	{
		UE_LOG(LogCustomizationComponent, Verbose, TEXT("No actors to add or update."));
		return;
	}

	UE_LOG(LogCustomizationComponent, Log, TEXT("Loading %d assets for actor spawning..."), AddedOrUpdatedAssetIds.Num());
	
	UCustomizationAssetManager::StaticSyncLoadAssetList<UCustomizationDataAsset>(
		AddedOrUpdatedAssetIds, [this, &TargetState, AssetIdToSlugMap, SlugToSlotMap](TArray<UCustomizationDataAsset*> LoadedAssets) // Removed counter capture, Pass TargetState by ref
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("Loaded %d assets for actor spawning."), LoadedAssets.Num());

			for (const auto& CustomizationAsset : LoadedAssets)
			{
				if (!CustomizationAsset) continue;

				const FPrimaryAssetId CustomizationAssetId = CustomizationAsset->GetPrimaryAssetId();
				const FName ItemSlug = AssetIdToSlugMap.FindChecked(CustomizationAssetId);
				const ECustomizationSlotType SlotType = SlugToSlotMap.FindChecked(ItemSlug);

				UE_LOG(LogCustomizationComponent, Verbose, TEXT("Spawning actors for ItemSlug: %s in Slot: %s"), *ItemSlug.ToString(), *UEnum::GetValueAsString(SlotType));

				TArray<TStrongObjectPtr<AActor>> SpawnedActorPtrs;
				TArray<AActor*> SpawnedActorsRaw;

				//const auto& SuitableComplects = CustomizationAsset->GetSuitableComplects(TargetState.Somatotype);
				// TODO:: maybe we'll need siutable assets depend on somatotype
				
				const auto& SuitableComplects = CustomizationAsset->CustomizationComplect;
				if (SuitableComplects.IsEmpty())
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("No suitable CustomizationComplect found in asset %s for item %s with Somatotype %s"),
						*CustomizationAssetId.ToString(), *ItemSlug.ToString(), *UEnum::GetValueAsString(TargetState.Somatotype));
					continue;
				}

				for (const FCustomizationComplect& Complect : SuitableComplects)
				{
					if (!Complect.ActorClass)
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalid ActorClass in Complect for item %s"), *ItemSlug.ToString());
						continue;
					}

					AActor* SpawnedActor = GetWorld()->SpawnActor(Complect.ActorClass);
					if (!SpawnedActor)
					{
						 UE_LOG(LogCustomizationComponent, Error, TEXT("Failed to spawn actor of class %s for item %s"), *Complect.ActorClass->GetName(), *ItemSlug.ToString());
						 continue;
					}

					SpawnedActorPtrs.Add(TStrongObjectPtr(SpawnedActor));
					SpawnedActorsRaw.Add(SpawnedActor);
					SpawnedActor->Tags.Add(GLOBAL_CONSTANTS::CustomizationTag);

					// 4. Attaching
					// Attach to BodySkin by default, could be made more flexible if needed
					if (USkeletalMeshComponent* AttachTarget = Skeletals.FindRef(EBodyPartType::BodySkin))
					{
						SpawnedActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Complect.SocketName);
						SpawnedActor->SetActorRelativeTransform(Complect.RelativeTransform);
						UE_LOG(LogCustomizationComponent, Verbose, TEXT("Attached spawned actor %s to %s at socket %s"), *SpawnedActor->GetName(), *AttachTarget->GetName(), *Complect.SocketName.ToString());
					}
					else
					{
						UE_LOG(LogCustomizationComponent, Warning, TEXT("Could not find BodySkin mesh component to attach actor %s"), *SpawnedActor->GetName());
					}
				}

				if (!SpawnedActorsRaw.IsEmpty())
				{
					OnNewCustomizationActorsAttached.Broadcast(SpawnedActorsRaw);

					// 5. Enable Physics (deferred to next tick)
					GetWorld()->GetTimerManager().SetTimerForNextTick([this, SpawnedActorsRaw]() mutable
					{
						for (AActor* SpawnedActor : SpawnedActorsRaw)
						{
							if (!SpawnedActor) continue;
							if (auto* CustomizationItemBase = Cast<ACustomizationItemBase>(SpawnedActor))
							{
								CustomizationItemBase->SetItemSimulatePhysics(true);
							}
						}
						SpawnedActorsRaw.Empty(); // Clear array after use in lambda
					});
				}

				// --- 6: Update TargetState info ---
				TargetState.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, SpawnedActorPtrs);
				UE_LOG(LogCustomizationComponent, Verbose, TEXT("Updated TargetState with spawned actors for %s"), *ItemSlug.ToString());
			}

			// Update Debug Info based on the *modified* TargetState
			DebugInfo.ActorInfo = TargetState.GetActorsList();
		});
	
	UE_LOG(LogCustomizationComponent, Log, TEXT("Finished Invalidating Attached Actors."));
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

void UCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwningCharacter = Cast<ABaseCharacter>(GetOwner());

	PendingInvalidationCounter.OnTriggered.AddWeakLambda(this, [this]
	{
		if (OwningCharacter.IsValid())
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidation Pipeline Completed. Broadcasting delegate."));
			OnInvalidationPipelineCompleted.Broadcast(OwningCharacter.Get());
		}
	});

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
		},0.1f, true, 0.f);
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
	Super::EndPlay(EndPlayReason);
}

void UCustomizationComponent::UpdateDebugInfo()
{
	if (!UCustomizationSettings::Get()->GetEnableDebug()) return;

	if (!OwningCharacter.IsValid()) return;
	
	const FVector CharacterLocation = OwningCharacter->GetMesh()->GetComponentLocation();
	//DrawDebugPoint(GetWorld(), CharacterLocation, 5.f, FColor::White, true, 0.1f, false);

	const FVector PendingItemsLocation	= CharacterLocation + FVector(0, 0, DebugInfo.VerticalOffset);																	// Left
	const FVector SkinCoverageLocation	= CharacterLocation + FVector(DebugInfo.HorizontalOffset, 0, DebugInfo.VerticalOffset);											// Above
	const FVector EquippedItemsLocation = CharacterLocation + FVector(DebugInfo.HorizontalOffset * 2, 0, DebugInfo.VerticalOffset);										// Right
	const FVector SkinInfoLocation		= CharacterLocation + FVector(0, 0, DebugInfo.VerticalOffset - DebugInfo.OffsetForSecondLine);									// Left, below 
	const FVector ActorInfoLocation		= CharacterLocation + FVector(DebugInfo.HorizontalOffset * -2, 0, DebugInfo.VerticalOffset - DebugInfo.OffsetForSecondLine);		// Right, below
	
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
