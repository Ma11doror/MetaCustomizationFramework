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
}

void UCustomizationComponent::UpdateFromOwning()
{
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

	CollectedNewContextData.Somatotype = OwningCharacter->Somatotype;

	HardRefreshAll();
}

void UCustomizationComponent::EquipItem(const FName& ItemSlug)
{
	if (ItemSlug == NAME_None)
	{
		return;
	}

	const FPrimaryAssetId ItemCustomizationAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
	//TODO:: log
	const auto CustomizationAssetClass = CustomizationUtilities::GetClassForCustomizationAsset(ItemCustomizationAssetId);

	//const auto EquipedSlugs = ContextData.GetEquippedSlugs();
	//if (EquipedSlugs.Contains(ItemSlug))
	//{
	//	return;
	//}

	if (CustomizationAssetClass == UCustomizationDataAsset::StaticClass())
	{
		const ECustomizationSlotType SlotType = CustomizationSlots::GetSlotTypeForItemBySlug(this, ItemSlug);
		if (OnlyOneItemInSlot)
		{
			const FEquippedItemActorsInfo EquippedItemActorsInfo = {ItemSlug, {}};
			const FEquippedItemsInSlotInfo EquippedItemsInSlotInfo = {{EquippedItemActorsInfo}};
			CollectedNewContextData.EquippedCustomizationItemActors.Emplace(SlotType, EquippedItemsInSlotInfo);
		}
		else
		{
			CollectedNewContextData.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, {});
		}
	}
	else if (CustomizationAssetClass == UMaterialPackCustomizationDA::StaticClass()
		|| CustomizationAssetClass == UMaterialCustomizationDataAsset::StaticClass())
	{
		CollectedNewContextData.EquippedMaterialsMap.Add(ItemSlug, EBodyPartType::None);
	}
	else if (CustomizationAssetClass == UBodyPartAsset::StaticClass())
	{
		CollectedNewContextData.EquippedBodyPartsItems.Add(ItemSlug, EBodyPartType::None);
	}
	Invalidate();
}

void UCustomizationComponent::EquipItems(const TArray<FName>& Items)
{
	for (const FName& ItemSlug : Items)
	{
		EquipItem(ItemSlug);
	}
}

void UCustomizationComponent::UnequipItem(const FName& ItemSlug)
{
	if (ItemSlug == NAME_None)
	{
		return;
	}

	const FPrimaryAssetId ItemCustomizationAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
	const auto CustomizationAssetClass = CustomizationUtilities::GetClassForCustomizationAsset(ItemCustomizationAssetId);

	if (CustomizationAssetClass == UCustomizationDataAsset::StaticClass())
	{
		const ECustomizationSlotType SlotType = CustomizationSlots::GetSlotTypeForItemBySlug(this, ItemSlug);
		if (OnlyOneItemInSlot)
		{
			CollectedNewContextData.EquippedCustomizationItemActors.Remove(SlotType);
		}
		else if (FEquippedItemsInSlotInfo* EquippedItemsInSlotInfo = CollectedNewContextData.EquippedCustomizationItemActors.Find(SlotType))
		{
			EquippedItemsInSlotInfo->EquippedItemActors.RemoveAll(
				[&ItemSlug](const FEquippedItemActorsInfo& EquippedItemActorsInfo) { return EquippedItemActorsInfo.ItemSlug == ItemSlug; });
		}
	}
	else if (CustomizationAssetClass == UMaterialPackCustomizationDA::StaticClass()
		|| CustomizationAssetClass == UMaterialCustomizationDataAsset::StaticClass())
	{
		CollectedNewContextData.EquippedMaterialsMap.Remove(ItemSlug);
	}
	else if (CustomizationAssetClass == UBodyPartAsset::StaticClass())
	{
		CollectedNewContextData.EquippedBodyPartsItems.Remove(ItemSlug);
	}
	Invalidate();
}

void UCustomizationComponent::ResetAll()
{
	ClearContext();
	Invalidate();
}

void UCustomizationComponent::HardRefreshAll()
{
	//TODO:: log
	for (auto& [PartType, Skeletal] : Skeletals)
	{
		//TODO:: log
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, PartType);
	}

	for (auto& [SlotType, ActorsInSlot] : CollectedNewContextData.EquippedCustomizationItemActors)
	{
		for (auto& ActorsInfo : ActorsInSlot.EquippedItemActors)
		{
			for (const auto& ItemRelatedActor : ActorsInfo.ItemRelatedActors)
			{
				//TODO:: log
				const FDetachmentTransformRules DetachmentTransformRules(
					EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
				ItemRelatedActor->DetachFromActor(DetachmentTransformRules);
				ItemRelatedActor->Destroy();
			}
		}
		ActorsInSlot.EquippedItemActors.Reset();
	}
	CollectedNewContextData.SkinVisibilityFlags.ClearAllFlags();

	//InvalidationContext.ClearAll();
	InvalidationContext.ClearTemporaryContext();
	Invalidate();
}

void UCustomizationComponent::SetCustomizationContext(const FCustomizationContextData& InContext)
{
	CollectedNewContextData = InContext;
	Invalidate();
}

const FCustomizationContextData& UCustomizationComponent::GetCustomizationContext()
{
	return CollectedNewContextData;
}

const TMap<EBodyPartType, USkeletalMeshComponent*>& UCustomizationComponent::GetSkeletals()
{
	return Skeletals;
}

void UCustomizationComponent::SetAttachedActorsSimulatePhysics(bool bSimulatePhysics)
{
}

ABaseCharacter* UCustomizationComponent::GetOwningCharacter()
{
	return OwningCharacter.IsValid() ? OwningCharacter.Get() : nullptr;
}

void UCustomizationComponent::ClearContext()
{
	HardRefreshAll();
	CollectedNewContextData = FCustomizationContextData();
	UpdateFromOwning();
}

void UCustomizationComponent::Invalidate(const bool bDeffer, ECustomizationInvalidationReason Reason)
{
	/*
	 * In deffer variant we should call invalidate only once on next tick
	 * This can help avoid multiple invalidation calls while change many properties
	 */
	if (bDeffer)
	{
		EnumAddFlags(DefferReason, Reason);
		//TODO:: log
		StartInvalidationTimer();
		return;
	}
	InvalidationContext.CheckDiff(CollectedNewContextData);
	EnumAddFlags(Reason, InvalidationContext.CalculateReason());

	//TODO:: log

	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Body))
	{
		PendingInvalidationCounter.Push();
		InvalidateBodyParts();
	}
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Actors))
	{
		InvalidateAttachedActors();
	}
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Skin))
	{
		// TODO:: see down
	}
	// Crutch. floating bug with initial skin flags. I just move from If(Reason == Skin) to this for now.
	PendingInvalidationCounter.Push();
	InvalidateSkin();
	
	FString MapStr;
	for (const auto& Pair : InvalidationContext.Current.EquippedBodyPartsItems)
	{
		MapStr += FString::Printf(TEXT(" || [%s: %d] "), *Pair.Key.ToString(), Pair.Value);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("InvalidationContext.Old Body parts equipped: %s"), *MapStr);
	
	InvalidationContext.ClearTemporaryContext();
	//CollectedNewContextData = FCustomizationContextData(); {};
}

void UCustomizationComponent::OnDefferInvalidationTimerExpired()
{
	//TODO:: log
	Invalidate(false, DefferReason);
	DefferReason = ECustomizationInvalidationReason::None;
}

void UCustomizationComponent::InvalidateSkin()
{
	auto ApplySkinToTarget = [this](UMaterialCustomizationDataAsset* MaterialCustomizationAsset, UMeshComponent* TargetMesh)
	{
		if (MaterialCustomizationAsset && TargetMesh)
		{
			CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, TargetMesh);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid MaterialCustomizationAsset or TargetMesh!"));
		}
	};

	auto ApplySkin = [this, ApplySkinToTarget](const FPrimaryAssetId& InSkinAssetId, EBodyPartType BodyPartType)
	{
		const FName SkinAssetTypeName = InSkinAssetId.PrimaryAssetType.GetName();
		if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialCustomizationDataAsset>(
				InSkinAssetId, [this, ApplySkinToTarget, SkinAssetTypeName, BodyPartType](UMaterialCustomizationDataAsset* MaterialCustomizationAsset)
				{
					if (!MaterialCustomizationAsset) {
						UE_LOG(LogTemp, Warning, TEXT("Failed to load MaterialCustomizationDataAsset!"));
						PendingInvalidationCounter.Pop();
						return;
					}

					if (MaterialCustomizationAsset->bApplyOnBodyPart)
					{
						if (MaterialCustomizationAsset->BodyPartType != BodyPartType && BodyPartType != EBodyPartType::None)
						{
							UE_LOG(LogTemp, Warning, TEXT("MaterialCustomizationAsset BodyPartType mismatch!"));
							PendingInvalidationCounter.Pop();
							return;
						}

						const auto TargetBodyPartType = MaterialCustomizationAsset->BodyPartType;
						if (Skeletals.Contains(TargetBodyPartType))
						{
							auto* TargetBodyPart = Skeletals[TargetBodyPartType];
							ApplySkinToTarget(MaterialCustomizationAsset, TargetBodyPart);
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("Body part %s not found!"), *UEnum::GetValueAsString(TargetBodyPartType));
						}
					}
					else
					{
						// Apply skin on Item related Actors
						for (const auto& [SlotType, EquippedCustomizationInfo] : CollectedNewContextData.EquippedCustomizationItemActors)
						{
							for (const auto& ActorsInfo : EquippedCustomizationInfo.EquippedItemActors)
							{
								for (const auto& Actor : ActorsInfo.ItemRelatedActors)
								{
									if (!Actor.IsValid()) continue;

									if (auto* StaticMesh = Actor->FindComponentByClass<UStaticMeshComponent>())
									{
										ApplySkinToTarget(MaterialCustomizationAsset, StaticMesh);
									}
									else if (auto* SkeletalMesh = Actor->FindComponentByClass<USkeletalMeshComponent>())
									{
										ApplySkinToTarget(MaterialCustomizationAsset, SkeletalMesh);
									}
								}
							}
						}
					}

					PendingInvalidationCounter.Pop();
				});
		}
		else if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialPackCustomizationDA>(
				InSkinAssetId, [this, ApplySkinToTarget, SkinAssetTypeName](UMaterialPackCustomizationDA* InSkinAsset)
				{
					if (!InSkinAsset) {
						UE_LOG(LogTemp, Warning, TEXT("Failed to load MaterialPackCustomizationDA!"));
						PendingInvalidationCounter.Pop();
						return;
					}

					const auto& AssetCollection = InSkinAsset->MaterialAsset;

					for (const auto& MaterialCustomizationAsset : AssetCollection.MaterialCustomizations)
					{
						if (!MaterialCustomizationAsset) continue;

						if (MaterialCustomizationAsset->bApplyOnBodyPart)
						{
							const auto TargetBodyPartType = MaterialCustomizationAsset->BodyPartType;
							if (Skeletals.Contains(TargetBodyPartType))
							{
								auto* TargetBodyPart = Skeletals[TargetBodyPartType];
								ApplySkinToTarget(MaterialCustomizationAsset, TargetBodyPart);
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("Body part %s not found!"), *UEnum::GetValueAsString(TargetBodyPartType));
							}
						}
						else
						{
							// Apply skin on Item related Actors
							for (const auto& [SlotType, EquippedCustomizationInfo] : CollectedNewContextData.EquippedCustomizationItemActors)
							{
								for (const auto& ActorsInfo : EquippedCustomizationInfo.EquippedItemActors)
								{
									for (const auto& Actor : ActorsInfo.ItemRelatedActors)
									{
										if (!Actor.IsValid()) continue;

										if (auto* StaticMesh = Actor->FindComponentByClass<UStaticMeshComponent>())
										{
											ApplySkinToTarget(MaterialCustomizationAsset, StaticMesh);
										}
										else if (auto* SkeletalMesh = Actor->FindComponentByClass<USkeletalMeshComponent>())
										{
											ApplySkinToTarget(MaterialCustomizationAsset, SkeletalMesh);
										}
									}
								}
							}
						}
					}

					CollectedNewContextData.VFXCustomization.CustomMaterial = InSkinAsset->CustomMaterial;
					PendingInvalidationCounter.Pop();
				});
		}
	};

	// Step 0: Apply default skin
	const FPrimaryAssetId DefaultSkin = UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(CollectedNewContextData.Somatotype);
	PendingInvalidationCounter.Push();
	ApplySkin(DefaultSkin, EBodyPartType::None);

	DebugInfo.SkinInfo = FString{};
	
	// Apply equipped skins
	for (const auto& [MaterialSlug, BodyPartType] : CollectedNewContextData.EquippedMaterialsMap)
	{
		FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(MaterialSlug);
		PendingInvalidationCounter.Push();
		ApplySkin(SkinAssetId, BodyPartType);
		
		DebugInfo.SkinInfo.Append(SkinAssetId.ToString() + " " + UEnum::GetValueAsString(BodyPartType) + "\n");
	}
	PendingInvalidationCounter.Pop();
}

void UCustomizationComponent::InvalidateBodyParts()
{
    const auto AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
    ensure(AssetManager);

    const FPrimaryAssetId AssetId = CustomizationUtilities::GetSomatotypeAssetId(CollectedNewContextData.Somatotype);
    if (!AssetId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Somatotype AssetId!"));
        return;
    }

    PendingInvalidationCounter.Push();

    AssetManager->SyncLoadAsset<USomatotypeDataAsset>(AssetId, [this](USomatotypeDataAsset* SomatotypeDataAsset)
    {
        if (!SomatotypeDataAsset) {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load SomatotypeDataAsset!"));
            PendingInvalidationCounter.Pop();
            return;
        }

        // 1. Prepare data structures
        TSet<FName> AddedSlugs;
        TSet<FName> RemovedSlugs;
        TMap<FName, EBodyPartType> SlugToType;  // Used for both Added and Current
        TArray<FPrimaryAssetId> ItemRelatedBodyPartAssetIds;
        const TArray<FPrimaryAssetId> EquippedItemAssetIds = CommonUtilities::ItemSlugsToAssetIds(CollectedNewContextData.GetEquippedSlugs());
        TArray<FPrimaryAssetId> IncludeOldEquippedItemAssetIds = CommonUtilities::ItemSlugsToAssetIds(InvalidationContext.Current.GetEquippedSlugs());
        IncludeOldEquippedItemAssetIds.Append(EquippedItemAssetIds);
    	
        // 2. Populate AddedSlugs, RemovedSlugs, and SlugToType from InvalidationContext
        for (const auto& [Slug, PartType] : InvalidationContext.Added.EquippedBodyPartsItems)
        {
            AddedSlugs.Add(Slug);
            SlugToType.Add(Slug, PartType);
            ItemRelatedBodyPartAssetIds.Add(CommonUtilities::ItemSlugToCustomizationAssetId(Slug));
        }

        for (const auto& [Slug, PartType] : InvalidationContext.Removed.EquippedBodyPartsItems)
        {
            RemovedSlugs.Add(Slug);
        }

        for (const auto& [Slug, PartType] : InvalidationContext.Current.EquippedBodyPartsItems)
        {
            SlugToType.Add(Slug, PartType);
        }

        // 
        LoadAndProcessBodyParts(SomatotypeDataAsset, AddedSlugs, RemovedSlugs, SlugToType, ItemRelatedBodyPartAssetIds, EquippedItemAssetIds, IncludeOldEquippedItemAssetIds);
    });
}

void UCustomizationComponent::LoadAndProcessBodyParts(USomatotypeDataAsset* SomatotypeDataAsset,
                                                      const TSet<FName>& AddedSlugs,
                                                      const TSet<FName>& RemovedSlugs,
                                                      const TMap<FName, EBodyPartType>& SlugToType,
                                                      const TArray<FPrimaryAssetId>& ItemRelatedBodyPartAssetIds,
                                                      const TArray<FPrimaryAssetId>& EquippedItemAssetIds,
                                                      const TArray<FPrimaryAssetId>& IncludeOldEquippedItemAssetIds)
{
	UCustomizationAssetManager::StaticSyncLoadAssetList<UBodyPartAsset>(ItemRelatedBodyPartAssetIds,
		[this, SomatotypeDataAsset, AddedSlugs, RemovedSlugs, SlugToType, EquippedItemAssetIds, IncludeOldEquippedItemAssetIds]
		(TArray<UBodyPartAsset*> ItemRelatedBodyParts)
		{
			// 3. Prepare mapping from slug to body part asset
			TMap<FName, UBodyPartAsset*> SlugToAsset;
			for (auto* ItemRelatedBodyPart : ItemRelatedBodyParts)
			{
				if (!ItemRelatedBodyPart) continue;
				FName Slug = ItemRelatedBodyPart->GetPrimaryAssetId().PrimaryAssetName;
				SlugToAsset.Add(Slug, ItemRelatedBodyPart);
			}

			// 4. Process removals
			ProcessRemovedBodyParts(RemovedSlugs, SlugToType, IncludeOldEquippedItemAssetIds, SlugToAsset);

			// 5. Process additions and detect conflicts
			ProcessAddedBodyParts(AddedSlugs, EquippedItemAssetIds, SlugToAsset);

			// 6. Apply default body parts
			TSet<EBodyPartType> UsedPartTypes;
			ApplyDefaultBodyParts(SomatotypeDataAsset, EquippedItemAssetIds, UsedPartTypes);

			// 7. Apply Skin
			ApplyBodySkin(SomatotypeDataAsset, UsedPartTypes);

			// 8. Reset unused body parts
			ResetUnusedBodyParts(UsedPartTypes);

			DebugInfo.EquippedItems = InvalidationContext.Current.GetItemsList();
			// 9. Update skin and broadcast
			PendingInvalidationCounter.Pop();
			Invalidate(true, ECustomizationInvalidationReason::Skin);
			OnSomatotypeLoaded.Broadcast(SomatotypeDataAsset);
		});
}

void UCustomizationComponent::ProcessRemovedBodyParts(const TSet<FName>& RemovedSlugs,
                                                      const TMap<FName, EBodyPartType>& SlugToType,
                                                      const TArray<FPrimaryAssetId>& IncludeOldEquippedItemAssetIds,
                                                      const TMap<FName, UBodyPartAsset*>& SlugToAsset)
{
	for (const FName& ItemSlug : RemovedSlugs)
	{
		if (!SlugToType.Contains(ItemSlug)) continue;

		EBodyPartType PartType = SlugToType.FindChecked(ItemSlug);
		UE_LOG(LogTemp, Warning, TEXT("Removing Item: %s, Type: %s"), *ItemSlug.ToString(), *UEnum::GetValueAsString(PartType));

		UBodyPartAsset* OldPartAsset = SlugToAsset.Contains(ItemSlug) ? SlugToAsset.FindChecked(ItemSlug) : nullptr;
		FBodyPartVariant* OldVariant = OldPartAsset ? OldPartAsset->GetMatchedVariant(IncludeOldEquippedItemAssetIds) : nullptr;

		if (OldVariant)
		{
			CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, OldVariant->BodyPartType);
			InvalidationContext.Current.EquippedBodyPartsItems.Remove(ItemSlug);
			CollectedNewContextData.EquippedBodyPartsItems.Remove(ItemSlug);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to find OldVariant for removal: %s"), *ItemSlug.ToString());
		}
	}
}

void UCustomizationComponent::ProcessAddedBodyParts(const TSet<FName>& AddedSlugs,
                                                    const TArray<FPrimaryAssetId>& EquippedItemAssetIds,
                                                    const TMap<FName, UBodyPartAsset*>& SlugToAsset)
{
	for (const FName& ItemSlug : AddedSlugs)
	{
		if (!SlugToAsset.Contains(ItemSlug)) continue;
		UBodyPartAsset* ItemRelatedBodyPart = SlugToAsset.FindChecked(ItemSlug);
		FBodyPartVariant* ItemBodyPartVariant = ItemRelatedBodyPart->GetMatchedVariant(EquippedItemAssetIds);

		if (!ItemBodyPartVariant || !ItemBodyPartVariant->IsValid()) continue;
		
		// Remove skin for this slot or material from old body part will be appltied to new item
		EBodyPartType EquippedPartType = ItemBodyPartVariant->BodyPartType;
		for(auto It = CollectedNewContextData.EquippedMaterialsMap.CreateIterator(); It; ++It)
		{
			if(It.Value() == EquippedPartType)
			{
				CollectedNewContextData.EquippedMaterialsMap.Remove(It.Key());
				break;
			}
		}

		// Check for conflicts (replace existing parts of the same type)
		for (auto& [ExistingSlug, ExistingType] : CollectedNewContextData.EquippedBodyPartsItems)
		{
			if (ExistingType == EquippedPartType && ExistingSlug != ItemSlug)
			{
				UE_LOG(LogTemp, Warning, TEXT("Conflict detected: Replacing %s (type %s) with %s"), *ExistingSlug.ToString(), *UEnum::GetValueAsString(ExistingType), *ItemSlug.ToString());

				// Remove the existing part from the equipped items
				CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, ExistingType);
				InvalidationContext.Current.EquippedBodyPartsItems.Remove(ExistingSlug);
				CollectedNewContextData.EquippedBodyPartsItems.Remove(ExistingSlug);
				break; // Only one conflict possible per added item
			}
		}

		// Apply the new body part
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, ItemBodyPartVariant->BodyPartSkeletalMesh, EquippedPartType);

		// Update context
		CollectedNewContextData.SkinVisibilityFlags.AddFlag(ItemBodyPartVariant->SkinCoverageFlags.FlagMask);
		CollectedNewContextData.EquippedBodyPartsItems.Add(ItemSlug, EquippedPartType);
		InvalidationContext.Current.EquippedBodyPartsItems.Add(ItemSlug, EquippedPartType);

		UE_LOG(LogTemp, Warning, TEXT("Adding Item: %s, Type: %s"), *ItemSlug.ToString(), *UEnum::GetValueAsString(EquippedPartType));
	}
}

void UCustomizationComponent::ApplyDefaultBodyParts(USomatotypeDataAsset* SomatotypeDataAsset, const TArray<FPrimaryAssetId>& EquippedItemAssetIds, TSet<EBodyPartType>& InUsedPartTypes)
{
	for (const auto& [Slug, PartType] : CollectedNewContextData.EquippedBodyPartsItems)
	{
		InUsedPartTypes.Add(PartType);
	}

	for (const auto& BodyPartAsset : SomatotypeDataAsset->BodyParts)
	{
		auto* BodyPartVariant = BodyPartAsset->GetMatchedVariant(EquippedItemAssetIds);
		if (!ensureAsRuntimeWarning(BodyPartVariant && BodyPartVariant->IsValid()))
		{
			continue;
		}

		if (InUsedPartTypes.Contains(BodyPartVariant->BodyPartType))
		{
			continue;
		}
		
		InUsedPartTypes.Add(BodyPartVariant->BodyPartType);
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, BodyPartVariant->BodyPartSkeletalMesh, BodyPartVariant->BodyPartType);

		// Update context
		FPrimaryAssetId BodyPartAssetId = BodyPartAsset->GetPrimaryAssetId();
		FName BodyPartSlug = BodyPartAssetId.PrimaryAssetName;

		InvalidationContext.Current.EquippedBodyPartsItems.Add(BodyPartSlug, BodyPartVariant->BodyPartType);
		CollectedNewContextData.EquippedBodyPartsItems.Add(BodyPartSlug, BodyPartVariant->BodyPartType);
	}
}

void UCustomizationComponent::ApplyBodySkin(USomatotypeDataAsset* SomatotypeDataAsset, TSet<EBodyPartType>& InUsedPartTypes)
{
	// Clear skin coverage flags and recollect it
	CollectedNewContextData.SkinVisibilityFlags.ClearAllFlags();

	for (const auto& [Slug, PartType] : CollectedNewContextData.EquippedBodyPartsItems)
	{
		const FPrimaryAssetId ItemAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(Slug);
		auto* BodyPartAsset = UCustomizationAssetManager::GetCustomizationAssetManager()->LoadBodyPartAssetSync(ItemAssetId);

		if (BodyPartAsset)
		{
			const TArray<FPrimaryAssetId> EquippedItemAssetIds = CommonUtilities::ItemSlugsToAssetIds(CollectedNewContextData.GetEquippedSlugs());
			if (auto* BodyPartVariant = BodyPartAsset->GetMatchedVariant(EquippedItemAssetIds))
			{
				CollectedNewContextData.SkinVisibilityFlags.AddFlag(BodyPartVariant->SkinCoverageFlags.FlagMask);
			}
		}
	}

	auto SkinMesh = CollectedNewContextData.SkinVisibilityFlags.GetMatch(SomatotypeDataAsset->SkinAssociation, CollectedNewContextData.SkinVisibilityFlags.FlagMask);
	if (SkinMesh)
	{
		InUsedPartTypes.Emplace(EBodyPartType::BodySkin);
		CustomizationUtilities::SetBodyPartSkeletalMesh(this, SkinMesh->BodyPartSkeletalMesh, EBodyPartType::BodySkin);
		DebugInfo.SkinCoverage = DebugInfo.FormatData(CollectedNewContextData.SkinVisibilityFlags);
	}
}

void UCustomizationComponent::ResetUnusedBodyParts(TSet<EBodyPartType>& InUsedPartTypes)
{
	for (const auto& [Slug, PartType] : CollectedNewContextData.EquippedBodyPartsItems)
	{
		UE_LOG(LogTemp, Warning, TEXT("Added to UsedPartTypes. Slug: %s, PartType: %s"), *Slug.ToString(), *UEnum::GetValueAsString(PartType));
		InUsedPartTypes.Add(PartType);
	}
	
	for (const auto& [PartType, Skeletal] : Skeletals)
	{
		if (!InUsedPartTypes.Contains(PartType) && Skeletal->GetSkeletalMeshAsset())
		{
			CustomizationUtilities::SetSkeletalMesh(this, nullptr, Skeletal);
		}
	}
}

void UCustomizationComponent::InvalidateAttachedActors()
{
	PendingInvalidationCounter.Push();

	// Step 0: Remove unused actors
	for (auto& [SlotType, ActorsInSlot] : InvalidationContext.Removed.EquippedCustomizationItemActors)
	{
		for (const auto& ActorsInfo : ActorsInSlot.EquippedItemActors)
		{
			for (const auto& ItemRelatedActor : ActorsInfo.ItemRelatedActors)
			{
				if (ItemRelatedActor.IsValid())
				{
					//TODO:: log
					const FDetachmentTransformRules DetachmentTransformRules(
						EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
					ItemRelatedActor->DetachFromActor(DetachmentTransformRules);
					ItemRelatedActor->Destroy();
				}
			}
		}
	}

	// Step 1: Gather Slugs to assets Load
	TArray<FPrimaryAssetId> AddedAssetIds;
	TMap<FPrimaryAssetId, FName> AddedAssetIdsToSlugs;
	for (const auto& [SlotType, ActorsInSlot] : InvalidationContext.Added.EquippedCustomizationItemActors)
	{
		for (const auto& ActorsInfo : ActorsInSlot.EquippedItemActors)
		{
			FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ActorsInfo.ItemSlug);
			AddedAssetIdsToSlugs.Emplace(AssetId, ActorsInfo.ItemSlug);
			AddedAssetIds.Emplace(AssetId);
		}
	}

	// Step 2: Load assets
	UCustomizationAssetManager::StaticSyncLoadAssetList<UCustomizationDataAsset>(
		AddedAssetIds, [this, AddedAssetIdsToSlugs](TArray<UCustomizationDataAsset*> ItemRelatedCustomizationAssets)
		{
			for (const auto& CustomizationAsset : ItemRelatedCustomizationAssets)
			{
				//				TODO:: log
				TArray<TStrongObjectPtr<AActor>> Spawned;

				// Step 3: Spawn added Actors
				const auto SuitableComplects = CustomizationAsset->CustomizationComplect;
				if (!ensureAsRuntimeWarning(SuitableComplects.Num() > 0))
				{
					//					TODO:: log
					continue;
				}

				TArray<AActor*> SpawnedActors;
				for (const FCustomizationComplect& Complect : SuitableComplects)
				{
					ensure(Complect.ActorClass);
					auto* SpawnedActor = GetWorld()->SpawnActor(Complect.ActorClass);
					ensure(SpawnedActor);
					Spawned.Add(TStrongObjectPtr(SpawnedActor));
					SpawnedActors.Add(SpawnedActor);
					SpawnedActor->Tags.Add(GLOBAL_CONSTANTS::CustomizationTag);
					ensure(Skeletals.Contains(EBodyPartType::BodySkin));
					SpawnedActor->AttachToComponent(
						Skeletals[EBodyPartType::BodySkin], FAttachmentTransformRules::KeepRelativeTransform, Complect.SocketName);
					SpawnedActor->SetActorRelativeTransform(Complect.RelativeTransform);
				}

				if (!SpawnedActors.IsEmpty())
				{
					OnNewCustomizationActorsAttached.Broadcast(SpawnedActors);

					// Enable physics on next tick to avoid jitters
					GetWorld()->GetTimerManager().SetTimerForNextTick([this, SpawnedActors]()
					{
						for (const auto& SpawnedActor : SpawnedActors)
						{
							if (auto* CustomizationItemBase = Cast<ACustomizationItemBase>(SpawnedActor))
							{
								CustomizationItemBase->SetItemSimulatePhysics(true);
							}
						}
					});
				}

				// Step 4: Fill Context info with SpawnedActors for EquippedCustomizationItemActors
				const FPrimaryAssetId CustomizationAssetId = CustomizationAsset->GetPrimaryAssetId();
				const FName ItemSlug = AddedAssetIdsToSlugs[CustomizationAssetId];
				const ECustomizationSlotType SlotType = CustomizationSlots::GetSlotTypeForItemBySlug(this, ItemSlug);

				CollectedNewContextData.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, Spawned);

				/*
				 * Update info of "Current" invalidation context. To proper actor destroying during next invalidation in the future.
				 * Because we operate with id's first, but didn't have actor's info
				 */
				InvalidationContext.Current.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, Spawned);
				DebugInfo.ActorInfo = InvalidationContext.Current.GetActorsList();
			}

			// Step 5: Invalidate Skin
			PendingInvalidationCounter.Pop();
			Invalidate(true, ECustomizationInvalidationReason::Skin);
		});
}

void UCustomizationComponent::StartInvalidationTimer()
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

	PendingInvalidationCounter.OnTriggered.AddWeakLambda(this, [this]
	{
		if (OwningCharacter.IsValid())
		{
			OnInvalidationPipelineCompleted.Broadcast(OwningCharacter.Get());
		}
	});

	if (IsActive())
	{
		UpdateFromOwning();
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
	if (InvalidationTimer)
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
	const FVector SkinCoverageLocation	= CharacterLocation + FVector(DebugInfo.HorizontalOffset, 0, DebugInfo.VerticalOffset);										// Above
	const FVector EquippedItemsLocation = CharacterLocation + FVector(DebugInfo.HorizontalOffset * 2, 0, DebugInfo.VerticalOffset);										// Right
	const FVector SkinInfoLocation		= CharacterLocation + FVector(0, 0, DebugInfo.VerticalOffset - DebugInfo.OffsetForSecondLine);									// Left, below 
	const FVector ActorInfoLocation		= CharacterLocation + FVector(DebugInfo.HorizontalOffset * 2, 0, DebugInfo.VerticalOffset - DebugInfo.OffsetForSecondLine);		// Right, below
	
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
