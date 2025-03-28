#include "AsyncCustomisation/Public/Components/CustomizationComponent.h"

#include "AsyncCustomisation/Public/BaseCharacter.h"
#include "AsyncCustomisation/Public/Components/Core/CustomizationUtilities.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "AsyncCustomisation/Public/Utilities/CommonUtilities.h"
#include "Components/Core/CustomizationItemBase.h"
#include "Components/Core/Assets/SomatotypeDataAsset.h"
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
		CollectedNewContextData.EquippedMaterialSlug = ItemSlug;
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
	//TODO:: log
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
		CollectedNewContextData.EquippedMaterialSlug = NAME_None;
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
	auto ApplySkin = [this](const FPrimaryAssetId& InSkinAssetId)
	{
		auto ApplyMaterialCustomizationAsset = [this](UMaterialCustomizationDataAsset* MaterialCustomizationAsset)
		{
			ensure(MaterialCustomizationAsset); //, TEXT("Invalid MaterialCustomizationAsset!"));

			if (MaterialCustomizationAsset->bApplyOnBodyPart)
			{
				// Apply skin on BodyPart
				const auto TargetBodyPartType = MaterialCustomizationAsset->BodyPartType;
				ensure(Skeletals.Contains(TargetBodyPartType));
				auto* TargetBodyPart = Skeletals[TargetBodyPartType];
				CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, TargetBodyPart);
			}
			else
			{
				// Apply skin on Item related Actors
				TArray<AActor*> AttachedActors;
				for (const auto& [SlotType, EquippedCustomizationInfo] : CollectedNewContextData.EquippedCustomizationItemActors)
				{
					for (const auto& ActorsInfo : EquippedCustomizationInfo.EquippedItemActors)
					{
						for (const auto& Actor : ActorsInfo.ItemRelatedActors)
						{
							if (!Actor.IsValid() || Actor->GetAttachParentSocketName() != MaterialCustomizationAsset->SocketName)
							{
								continue;
							}

							if (auto* StaticMesh = Actor->FindComponentByClass<UStaticMeshComponent>())
							{
								CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, StaticMesh);
							}
							else if (auto* SkeletalMesh = Actor->FindComponentByClass<USkeletalMeshComponent>())
							{
								CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, SkeletalMesh);
							}
						}
					}
				}
			}
		};

		const FName SkinAssetTypeName = InSkinAssetId.PrimaryAssetType.GetName();
		if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialCustomizationDataAsset>(
				InSkinAssetId, [this, ApplyMaterialCustomizationAsset](UMaterialCustomizationDataAsset* MaterialCustomizationAsset)
				{
					ApplyMaterialCustomizationAsset(MaterialCustomizationAsset);
					PendingInvalidationCounter.Pop();
				});
		}
		else if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialPackCustomizationDA>(
				InSkinAssetId, [this, ApplyMaterialCustomizationAsset](UMaterialPackCustomizationDA* InSkinAsset)
				{
					const auto AssetCollection = InSkinAsset->MaterialAsset;
					//IsValid(AssetCollection);
					for (const auto& MaterialCustomizationAsset : AssetCollection.MaterialCustomizations)
					{
						if (!MaterialCustomizationAsset
							|| CollectedNewContextData.EquippedBodyPartsItems.FindKey(MaterialCustomizationAsset->BodyPartType))
						{
							continue; /*Ignore Item related body parts*/
						}
						ApplyMaterialCustomizationAsset(MaterialCustomizationAsset);
					}
					CollectedNewContextData.VFXCustomization.CustomFeatherMaterial = InSkinAsset->CustomFeathersMaterial;
					PendingInvalidationCounter.Pop();
				});
		}
	};

	// Step 0: Apply default skin
	const FPrimaryAssetId DefaultSkin = UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(CollectedNewContextData.Somatotype);
	//TODO:: log
	PendingInvalidationCounter.Push();
	ApplySkin(DefaultSkin);

	//TODO:: log
	if (CollectedNewContextData.EquippedMaterialSlug != NAME_None)
	{
		FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(CollectedNewContextData.EquippedMaterialSlug);
		//TODO:: log
		PendingInvalidationCounter.Push();
		ApplySkin(SkinAssetId);
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
		//TODO:: log
		return;
	}

	PendingInvalidationCounter.Push();

	// Step 0: Load Somatotype Data Asset
	AssetManager->SyncLoadAsset<USomatotypeDataAsset>(AssetId, [this](USomatotypeDataAsset* SomatotypeDataAsset)
	{
		// Step 1: Gather Item AssetId associations
		TMap<FPrimaryAssetId, FName> AddedAssetIdsToSlugs;
		TArray<FPrimaryAssetId> ItemRelatedBodyPartIds;
		
		for (const auto& [ItemSlug, BodyPartType] : InvalidationContext.Added.EquippedBodyPartsItems)
		{
			FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
			AddedAssetIdsToSlugs.Emplace(AssetId, ItemSlug);
			ItemRelatedBodyPartIds.Emplace(AssetId);
		}

		// Step 2: Deffer Apply item related body parts. Only added.
		UCustomizationAssetManager::StaticSyncLoadAssetList<UBodyPartAsset>(ItemRelatedBodyPartIds,
			[this,
				SomatotypeDataAsset,
				DataAsset = TStrongObjectPtr(SomatotypeDataAsset),
				AddedAssetIdsToSlugs]
			(TArray<UBodyPartAsset*> ItemRelatedBodyParts)
			{
				// Step 3: Get Equipped Item AssetIds (Not Customization assets) to match BodyPartVariant
				const TArray<FPrimaryAssetId> EquippedItemAssetIds = CommonUtilities::ItemSlugsToAssetIds(CollectedNewContextData.GetEquippedSlugs());

				// Step 4: Processing new items. Get theirs real types
				UE_LOG(LogTemp, Warning, TEXT("InvalidateBodyParts: Determining types for new items"));
				
				TMap<FName, EBodyPartType> AddedItemsRealTypes;
				for (const auto& ItemRelatedBodyPart : ItemRelatedBodyParts)
				{
					const FPrimaryAssetId PrimaryAssetId = ItemRelatedBodyPart->GetPrimaryAssetId();
					const FName Slug = AddedAssetIdsToSlugs[PrimaryAssetId];
                    
					auto* ItemBodyPartVariant = ItemRelatedBodyPart->GetMatchedVariant(EquippedItemAssetIds);
					if (ItemBodyPartVariant && ItemBodyPartVariant->IsValid())
					{
						AddedItemsRealTypes.Add(Slug, ItemBodyPartVariant->BodyPartType);
					}

					UE_LOG(LogTemp, Warning, TEXT("Item %s Got type %s"),
					*Slug.ToString(),
					*UEnum::GetValueAsString(ItemBodyPartVariant->BodyPartType));
				}
				// Step 5: check if we have conflicting items
				// if there are items with same type - add them to remove list
				TArray<FPrimaryAssetId> IncludeOldEquippedItemAssetIds = CommonUtilities::ItemSlugsToAssetIds(InvalidationContext.Current.GetEquippedSlugs());
				IncludeOldEquippedItemAssetIds.Append(EquippedItemAssetIds);
				
				TMap<FName, EBodyPartType> ConflictRemoves;
				
				for (const auto& [NewItemSlug, NewItemType] : AddedItemsRealTypes)
				{
					for (const auto& [ExistingSlug, ExistingType] : InvalidationContext.Current.EquippedBodyPartsItems)
					{
						if (ExistingType == NewItemType && ExistingSlug != NewItemSlug)
						{
							ConflictRemoves.Add(ExistingSlug, ExistingType);
							UE_LOG(LogTemp, Warning, TEXT("Found conflict: Item %s of type %s conflicts with new item %s"),
								*ExistingSlug.ToString(),
								*UEnum::GetValueAsString(ExistingType),
								*NewItemSlug.ToString());
						}
					}
				}
				
				for (const auto& [RemoveSlug, RemoveType] : ConflictRemoves)
				{
					InvalidationContext.Removed.EquippedBodyPartsItems.Add(RemoveSlug, RemoveType);
				}
				// Step 6: Processing Removed parts
				UE_LOG(LogTemp, Warning, TEXT("InvalidateBodyParts: Processing removed items"));
				for (const auto& [ItemSlug, PartType] : InvalidationContext.Removed.EquippedBodyPartsItems)
				{
					UE_LOG(LogTemp, Warning, TEXT("Removing Item: %s, Type: %s"), *ItemSlug.ToString(), *UEnum::GetValueAsString(PartType));

					auto OldPartAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(ItemSlug);
					auto* OldPartAsset = UCustomizationAssetManager::GetCustomizationAssetManager()->LoadBodyPartAssetSync(OldPartAssetId);
					auto* OldVariant = OldPartAsset ? OldPartAsset->GetMatchedVariant(IncludeOldEquippedItemAssetIds) : nullptr;
                    
					if (OldPartAsset && OldVariant)
					{
						CustomizationUtilities::SetBodyPartSkeletalMesh(this, nullptr, OldVariant->BodyPartType);
						//TODO:: log
						InvalidationContext.Current.EquippedBodyPartsItems.Remove(ItemSlug);
						CollectedNewContextData.EquippedBodyPartsItems.Remove(ItemSlug);
					}
				}
				
				// Step 6.5: Clear skin coverage flags and recollect it 
				CollectedNewContextData.SkinVisibilityFlags.ClearAllFlags();
				
                // Step 7: Apply added parts
				
                UE_LOG(LogTemp, Warning, TEXT("InvalidateBodyParts: Applying new items"));
                for (const auto& ItemRelatedBodyPart : ItemRelatedBodyParts)
                {
                    const FPrimaryAssetId AssetId = ItemRelatedBodyPart->GetPrimaryAssetId();
                    const FName Slug = AddedAssetIdsToSlugs[AssetId];
                    
                    auto* ItemBodyPartVariant = ItemRelatedBodyPart->GetMatchedVariant(EquippedItemAssetIds);
                    if (!ItemBodyPartVariant || !ItemBodyPartVariant->IsValid())
                    {
                        continue;
                    }
                    
                    EBodyPartType RealPartType = ItemBodyPartVariant->BodyPartType;
                    
                    // check for existing or conflicting item

                    for (const auto& [ExistingSlug, ExistingType] : InvalidationContext.Current.EquippedBodyPartsItems)
                    {
                        if (ExistingType == RealPartType && ExistingSlug != Slug)
                        {
                        	UE_LOG(LogTemp, Warning, TEXT("For some reason we have a conflict for item %s of type %s, skipping"),
							   *Slug.ToString(),
							   *UEnum::GetValueAsString(RealPartType));
                            break;
                        }
                    }
                    
                    
                    CustomizationUtilities::SetBodyPartSkeletalMesh(this, ItemBodyPartVariant->BodyPartSkeletalMesh, RealPartType);
                    
                    // Update context
                    CollectedNewContextData.SkinVisibilityFlags.AddFlag(ItemBodyPartVariant->SkinCoverageFlags.FlagMask);
                    CollectedNewContextData.EquippedBodyPartsItems.Add(Slug, RealPartType);
                    InvalidationContext.Current.EquippedBodyPartsItems.Add(Slug, RealPartType);
                }
				// Step 8: Apply default body parts
				TArray<EBodyPartType> UsedPartTypes;
				CollectedNewContextData.EquippedBodyPartsItems.GenerateValueArray(UsedPartTypes);
				for (const auto& BodyPartAsset : DataAsset->BodyParts)
				{
					auto* BodyPartVariant = BodyPartAsset->GetMatchedVariant(EquippedItemAssetIds);
					if (!ensureAsRuntimeWarning(BodyPartVariant && BodyPartVariant->IsValid()))
					{
						continue;
					}
                
					// If slot is used - not apply default body parts
					if (CollectedNewContextData.EquippedBodyPartsItems.FindKey(BodyPartVariant->BodyPartType))
					{
						continue;
					}

					UsedPartTypes.Emplace(BodyPartVariant->BodyPartType);
					CustomizationUtilities::SetBodyPartSkeletalMesh(this, BodyPartVariant->BodyPartSkeletalMesh, BodyPartVariant->BodyPartType);
                
					// Fill context
					// TODO:: check if we really need to add to both maps or just to current
					FPrimaryAssetId BodyPartAssetId = BodyPartAsset->GetPrimaryAssetId();
					FName BodyPartSlug = BodyPartAssetId.PrimaryAssetName;
					UE_LOG(LogTemp, Warning, TEXT("Default BodyPartSlug %s "), *BodyPartSlug.ToString());
                
					InvalidationContext.Current.EquippedBodyPartsItems.Add(BodyPartSlug, BodyPartVariant->BodyPartType);
					CollectedNewContextData.EquippedBodyPartsItems.Add(BodyPartSlug, BodyPartVariant->BodyPartType);
				}

                // Step 9: Apply Body skin depend on body part flags
                auto SkinMesh = CollectedNewContextData.SkinVisibilityFlags.GetMatch(DataAsset->SkinAssociation, CollectedNewContextData.SkinVisibilityFlags.FlagMask);
                if (SkinMesh)
                {
                    UsedPartTypes.Emplace(EBodyPartType::BodySkin);
                    CustomizationUtilities::SetBodyPartSkeletalMesh(this, SkinMesh->BodyPartSkeletalMesh, EBodyPartType::BodySkin);
                	UE_LOG(LogTemp, Warning, TEXT("Current skin flags: %s"), *CollectedNewContextData.SkinVisibilityFlags.UpdateDescription());
                }

                // Step 10: Reset unused BodyPart skeletals (different somatotypes may have variation in body parts set)
                for (const auto& [PartType, Skeletal] : Skeletals)
                {
                    if (!UsedPartTypes.Contains(PartType) && Skeletal->GetSkeletalMeshAsset())
                    {
                        CustomizationUtilities::SetSkeletalMesh(this, nullptr, Skeletal);
                        //TODO:: log
                    }
                }

                // Step 11: Invalidate Skin
                PendingInvalidationCounter.Pop();
                Invalidate(true, ECustomizationInvalidationReason::Skin);
                OnSomatotypeLoaded.Broadcast(SomatotypeDataAsset);
			});
	});
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
