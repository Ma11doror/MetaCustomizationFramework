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
        // TODO: Handle if something will be wrong with packs of materisl?
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

ABaseCharacter* UCustomizationComponent::GetOwningCharacter()
{
	return OwningCharacter.IsValid() ? OwningCharacter.Get() : nullptr;
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
	FCustomizationContextData ModifiableTargetState  = TargetState;
	
    UE_LOG(LogCustomizationComponent, Log, TEXT("Starting Immediate Invalidation..."));

    // 1. Get diff (CurrentCustomizationState) and (TargetState)
    InvalidationContext.Current = CurrentCustomizationState;
    InvalidationContext.CheckDiff(ModifiableTargetState);

    // 2. 
	ECustomizationInvalidationReason Reason = ExplicitReason;
	EnumAddFlags(Reason, InvalidationContext.CalculateReason());

	if (Reason == ECustomizationInvalidationReason::None)
	{
		UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidation skipped: No reason found."));
         if (CurrentCustomizationState != TargetState)
         {
              CurrentCustomizationState = TargetState;
         }
        InvalidationContext.ClearTemporaryContext();
		DeferredTargetState.Reset();
		return;
	}

	UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidation Reason: %s"), *StaticEnum<ECustomizationInvalidationReason>()->GetNameStringByValue(static_cast<int64>(Reason)));

    // 3. 
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Body))
	{
		PendingInvalidationCounter.Push();
		InvalidateBodyParts(ModifiableTargetState); 
	}
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Actors))
	{
		PendingInvalidationCounter.Push();
		InvalidateAttachedActors(ModifiableTargetState);
	}
	if (EnumHasAnyFlags(Reason, ECustomizationInvalidationReason::Skin))
	{
		PendingInvalidationCounter.Push();
		InvalidateSkin(ModifiableTargetState);

		//TODO:: Check if it flow needed cz we call invalidate skin inside InvalidateBodyParts() 
	}
	
	FString MapStr;
	for (const auto& Pair : ModifiableTargetState.EquippedBodyPartsItems)
	{
		MapStr += FString::Printf(TEXT(" || [%s: %s] "), *Pair.Key.ToString(), *UEnum::GetValueAsString(Pair.Value));
	}
	UE_LOG(LogCustomizationComponent, Log, TEXT("Target Body parts applied (final): %s"), *MapStr);

    // 4. update current to target state after all invalidates
    CurrentCustomizationState = TargetState;
	
	InvalidationContext.ClearTemporaryContext();
	
    DeferredTargetState.Reset();
	CurrentCustomizationState = ModifiableTargetState;
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

	auto ApplySkinToTarget = [this](UMaterialCustomizationDataAsset* MaterialCustomizationAsset, USkeletalMeshComponent* TargetMesh) // Работаем только с SkeletalMeshComponent частей тела
	{
		if (MaterialCustomizationAsset && TargetMesh)
		{
            UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying material %s to mesh %s"), *MaterialCustomizationAsset->GetPrimaryAssetId().ToString(), *TargetMesh->GetName());
			CustomizationUtilities::SetMaterialOnMesh(MaterialCustomizationAsset, TargetMesh);
		}
	};

	auto ApplySkin = [this, ApplySkinToTarget, &TargetState](const FPrimaryAssetId& InSkinAssetId)
	{
		const FName SkinAssetTypeName = InSkinAssetId.PrimaryAssetType.GetName();
		if (SkinAssetTypeName == GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType)
		{
			UCustomizationAssetManager::StaticSyncLoadAsset<UMaterialCustomizationDataAsset>(
				InSkinAssetId, [this, ApplySkinToTarget, InSkinAssetId](UMaterialCustomizationDataAsset* MaterialCustomizationAsset)
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
				InSkinAssetId, [this, ApplySkinToTarget, InSkinAssetId](UMaterialPackCustomizationDA* InSkinAsset)
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
                        // Update VFX in "current" state after invalidation
                    }
                    else
                    {
	                    UE_LOG(LogCustomizationComponent, Warning, TEXT("Failed to load MaterialPackCustomizationDA: %s"), *InSkinAssetId.ToString());
                    }
					PendingInvalidationCounter.Pop(); 
				});
		}
	};

	// --- InvalidateSkin ---
	// Step 0: Apply default skin (TargetState)
	const FPrimaryAssetId DefaultSkin = UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(TargetState.Somatotype);
    if (DefaultSkin.IsValid())
    {
    	PendingInvalidationCounter.Push();
	    ApplySkin(DefaultSkin);
    }
    else
    {
        UE_LOG(LogCustomizationComponent, Warning, TEXT("No Default Skin found for Somatotype: %s"), *UEnum::GetValueAsString(TargetState.Somatotype));
        // Возможно, нужно сбросить материалы на стандартные из мешей?
        for(auto const& [BodyPartType, SkeletalComp] : Skeletals)
        {
            if (SkeletalComp && SkeletalComp->GetSkeletalMeshAsset())
            {
                int32 NumMaterials = SkeletalComp->GetMaterials().Num();
                for (int32 i = 0; i < NumMaterials; ++i)
                {
                     SkeletalComp->SetMaterial(i, SkeletalComp->GetSkeletalMeshAsset()->GetMaterials()[i].MaterialInterface);
                }
            }
        }
    }

	DebugInfo.SkinInfo = FString{};

	// Apply equipped skins (TargetState)
	for (const auto& [MaterialSlug, BodyPartType] : TargetState.EquippedMaterialsMap) // BodyPartType здесь не используется напрямую
	{
		FPrimaryAssetId SkinAssetId = CommonUtilities::ItemSlugToCustomizationAssetId(MaterialSlug);
        if (SkinAssetId.IsValid())
        {
        	PendingInvalidationCounter.Push();
		    ApplySkin(SkinAssetId);
		    DebugInfo.SkinInfo.Append(SkinAssetId.ToString() + "\n"); // Убрали BodyPartType отсюда
        }
	}
    UE_LOG(LogCustomizationComponent, Log, TEXT("Finished Invalidating Skin."));
}

void UCustomizationComponent::InvalidateBodyParts(FCustomizationContextData& TargetState)
{
    UE_LOG(LogCustomizationComponent, Log, TEXT("Invalidating Body Parts based on TargetState..."));
    PendingInvalidationCounter.Push(); // Счетчик на всю функцию

    const auto AssetManager = UCustomizationAssetManager::GetCustomizationAssetManager();
    if (!AssetManager) {
         UE_LOG(LogCustomizationComponent, Error, TEXT("AssetManager is null!"));
         PendingInvalidationCounter.Pop();
         return;
    }

    const FPrimaryAssetId SomatotypeAssetId = CustomizationUtilities::GetSomatotypeAssetId(TargetState.Somatotype);
    if (!SomatotypeAssetId.IsValid())
    {
        UE_LOG(LogCustomizationComponent, Warning, TEXT("Invalid Somatotype AssetId!"));
        PendingInvalidationCounter.Pop();
        return;
    }

    PendingInvalidationCounter.Push();
    AssetManager->SyncLoadAsset<USomatotypeDataAsset>(SomatotypeAssetId, [this, &TargetState](USomatotypeDataAsset* SomatotypeDataAsset)
    {
        if (!SomatotypeDataAsset) {
            UE_LOG(LogCustomizationComponent, Warning, TEXT("Failed to load SomatotypeDataAsset!"));
        	PendingInvalidationCounter.Pop(); 
			PendingInvalidationCounter.Pop();
            return;
        }

        TSet<FPrimaryAssetId> RelevantAssetIdsSet;
        TArray<FName> AddedSlugs;
        TArray<FName> RemovedSlugs;

        // InvalidationContext
        for (const auto& [Slug, PartType] : InvalidationContext.Added.EquippedBodyPartsItems)
        {
	        RelevantAssetIdsSet.Add(CommonUtilities::ItemSlugToCustomizationAssetId(Slug));
	        AddedSlugs.Add(Slug);
        }
        // InvalidationContext
        for (const auto& [Slug, PartType] : InvalidationContext.Removed.EquippedBodyPartsItems)
        {
	        RelevantAssetIdsSet.Add(CommonUtilities::ItemSlugToCustomizationAssetId(Slug));
	        RemovedSlugs.Add(Slug);
        }
        // Default
        for (const auto& BodyPartAssetPtr : SomatotypeDataAsset->BodyParts)
        {
	        if (BodyPartAssetPtr) RelevantAssetIdsSet.Add(BodyPartAssetPtr->GetPrimaryAssetId());
        }

        for (const auto& [Slug, PartType] : TargetState.EquippedBodyPartsItems)
        {
	        RelevantAssetIdsSet.Add(CommonUtilities::ItemSlugToCustomizationAssetId(Slug));
        }

        TArray<FPrimaryAssetId> AllRelevantItemAssetIds = RelevantAssetIdsSet.Array();

        //  IDs for target state
        const TArray<FPrimaryAssetId> TargetEquippedItemAssetIds = CommonUtilities::ItemSlugsToAssetIds(TargetState.GetEquippedSlugs());
    	
        LoadAndProcessBodyParts(TargetState, SomatotypeDataAsset, AllRelevantItemAssetIds);
    	PendingInvalidationCounter.Pop();
    });
	
    UE_LOG(LogCustomizationComponent, Log, TEXT("Finished Invalidating Body Parts."));
}

void UCustomizationComponent::LoadAndProcessBodyParts(FCustomizationContextData& TargetState,
                                                      USomatotypeDataAsset* SomatotypeDataAsset,
                                                      TArray<FPrimaryAssetId>& AllRelevantItemAssetIds)
{
	UE_LOG(LogCustomizationComponent, Log, TEXT("Loading and Processing %d Body Part Assets for replacement logic..."), AllRelevantItemAssetIds.Num());
	PendingInvalidationCounter.Push(); // Счетчик на загрузку ассетов

	UCustomizationAssetManager::StaticSyncLoadAssetList<UBodyPartAsset>(AllRelevantItemAssetIds,
		[this, &TargetState, SomatotypeDataAsset] // Захватываем TargetState по ссылке
		(TArray<UBodyPartAsset*> LoadedBodyParts)
		{
			UE_LOG(LogCustomizationComponent, Log, TEXT("Loaded %d Body Part Assets."), LoadedBodyParts.Num());

			if (!SomatotypeDataAsset) {
				UE_LOG(LogCustomizationComponent, Error, TEXT("SomatotypeDataAsset is null during processing!"));
				PendingInvalidationCounter.Pop(); // Pop for StaticSyncLoadAssetList lambda
				// Removed second pop, assuming the outer function call (LoadAndProcessBodyParts) should handle its own pop if needed.
				// Let's re-evaluate counter logic if issues arise.
				return;
			}

			TMap<FName, UBodyPartAsset*> SlugToAssetMap;
			for (auto* BodyPartAsset : LoadedBodyParts) {
				if (BodyPartAsset) SlugToAssetMap.Add(BodyPartAsset->GetPrimaryAssetId().PrimaryAssetName, BodyPartAsset);
			}

			// --- Шаг 1: Определяем финальное назначение слотов ---
			TMap<EBodyPartType, FName> FinalSlotAssignment; // Карта: ТипЧастиТела -> Слаг Победителя
			TMap<FName, const FBodyPartVariant*> AllValidVariants; // Кешируем найденные валидные варианты

			// Сохраняем исходный список явно запрошенных слагов
			TArray<FName> ExplicitlyRequestedSlugs;
			TargetState.EquippedBodyPartsItems.GenerateKeyArray(ExplicitlyRequestedSlugs);

			// Получаем AssetId для текущего *намерения* экипировки (из TargetState)
            // Это важно для GetMatchedVariant, если варианты зависят от других экипированных предметов.
			TArray<FPrimaryAssetId> IntendedEquipmentAssetIds = CommonUtilities::ItemSlugsToAssetIds(TargetState.GetEquippedSlugs());

			// Функция для поиска валидного варианта и его типа
			auto GetVariantInfo = [&](const FName& Slug, EBodyPartType& OutType, const FBodyPartVariant*& OutVariant) -> bool {
				if (const FBodyPartVariant** CachedVariant = AllValidVariants.Find(Slug))
                {
                    OutVariant = *CachedVariant; // Используем кеш, если есть
                }
                else if (UBodyPartAsset** FoundAssetPtr = SlugToAssetMap.Find(Slug))
				{
					UBodyPartAsset* Asset = *FoundAssetPtr;
					if (!Asset) return false;
					// Используем IntendedEquipmentAssetIds для поиска варианта
					OutVariant = Asset->GetMatchedVariant(IntendedEquipmentAssetIds);
					if (OutVariant && OutVariant->IsValid()) {
                        AllValidVariants.Add(Slug, OutVariant); // Кешируем найденный вариант
					} else {
                        OutVariant = nullptr; // Невалидный вариант
                    }
				} else {
                    OutVariant = nullptr; // Ассет не загружен
                }

                if(OutVariant) {
                    OutType = OutVariant->BodyPartType;
                    return true; // Валидный вариант найден
                } else {
                    OutType = EBodyPartType::None;
                    return false; // Валидный вариант НЕ найден
                }
			};

			// Обрабатываем явно запрошенные предметы (из TargetState)
			for (const FName& Slug : ExplicitlyRequestedSlugs)
			{
				EBodyPartType ItemBodyPartType = EBodyPartType::None;
				const FBodyPartVariant* ItemVariant = nullptr;

				if (GetVariantInfo(Slug, ItemBodyPartType, ItemVariant)) // Нашли валидный вариант
				{
					if (ItemBodyPartType != EBodyPartType::None)
					{
						// Этот предмет назначается на слот, заменяя любой предыдущий
						FinalSlotAssignment.Add(ItemBodyPartType, Slug);
						UE_LOG(LogCustomizationComponent, Verbose, TEXT("Assigning BodyPartType %s to explicit item '%s' (replacing previous if any)."),
							*UEnum::GetValueAsString(ItemBodyPartType), *Slug.ToString());
					}
					else {
						UE_LOG(LogCustomizationComponent, Warning, TEXT("Explicit item '%s' resolved to BodyPartType::None. Skipping assignment."), *Slug.ToString());
					}
				}
				else {
					UE_LOG(LogCustomizationComponent, Warning, TEXT("No valid variant found for explicit item '%s'. Skipping assignment."), *Slug.ToString());
				}
			}

			// Обрабатываем дефолтные предметы (из соматотипа)
			for (const auto& BodyPartAssetPtr : SomatotypeDataAsset->BodyParts)
			{
				if (!BodyPartAssetPtr) continue;
				FName DefaultSlug = BodyPartAssetPtr->GetPrimaryAssetId().PrimaryAssetName;

				EBodyPartType ItemBodyPartType = EBodyPartType::None;
				const FBodyPartVariant* ItemVariant = nullptr;

				if (GetVariantInfo(DefaultSlug, ItemBodyPartType, ItemVariant)) // Нашли валидный вариант
				{
					if (ItemBodyPartType != EBodyPartType::None)
					{
						// Назначаем дефолтный предмет, только если слот еще не занят явным предметом
						if (!FinalSlotAssignment.Contains(ItemBodyPartType))
						{
							FinalSlotAssignment.Add(ItemBodyPartType, DefaultSlug);
							UE_LOG(LogCustomizationComponent, Verbose, TEXT("Assigning BodyPartType %s to default item '%s' (slot was empty)."),
								*UEnum::GetValueAsString(ItemBodyPartType), *DefaultSlug.ToString());
						}
                        // else { UE_LOG(LogCustomizationComponent, Verbose, TEXT("BodyPartType %s already assigned by explicit item. Skipping default '%s'."), *UEnum::GetValueAsString(ItemBodyPartType), *DefaultSlug.ToString()); }
					}
                    // else { UE_LOG(LogCustomizationComponent, Warning, TEXT("Default item '%s' resolved to BodyPartType::None."), *DefaultSlug.ToString()); }
				}
                // else { UE_LOG(LogCustomizationComponent, Warning, TEXT("No valid variant found for default item '%s'."), *DefaultSlug.ToString()); }
			}

			// --- Шаг 2: Пересобираем TargetState.EquippedBodyPartsItems ---
			TargetState.EquippedBodyPartsItems.Empty(); // Очищаем старую карту
			TMap<FName, const FBodyPartVariant*> FinalSlugToVariantMap; // Для применения мешей
			TSet<EBodyPartType> FinalUsedPartTypes;      // Для сброса неиспользуемых и скинов
            TArray<FName> FinalActiveSlugs;           // Для применения мешей

			UE_LOG(LogCustomizationComponent, Log, TEXT("Rebuilding TargetState based on final slot assignments..."));
			for (const auto& Pair : FinalSlotAssignment)
			{
				EBodyPartType BodyPartType = Pair.Key;
				FName Slug = Pair.Value;

				if (const FBodyPartVariant* const* FoundVariant = AllValidVariants.Find(Slug)) // Используем кеш вариантов
				{
					// Добавляем победителя в итоговое состояние с правильным типом
					TargetState.EquippedBodyPartsItems.Add(Slug, BodyPartType);

					// Собираем данные для применения мешей и скинов
					FinalSlugToVariantMap.Add(Slug, *FoundVariant);
					FinalUsedPartTypes.Add(BodyPartType);
                    FinalActiveSlugs.Add(Slug);
                     UE_LOG(LogCustomizationComponent, Verbose, TEXT("Final state includes: Slug='%s', Type='%s'"), *Slug.ToString(), *UEnum::GetValueAsString(BodyPartType));
				}
                else
                {
                    // Эта ситуация не должна возникать, если вариант был найден ранее
                    UE_LOG(LogCustomizationComponent, Error, TEXT("Could not find cached variant for final item '%s'. State might be incorrect."), *Slug.ToString());
                }
			}

			// --- Шаг 3: Применяем скин тела ---
			// Передаем FinalUsedPartTypes по ссылке, т.к. ApplyBodySkin может добавить EBodyPartType::BodySkin
			ApplyBodySkin(TargetState, SomatotypeDataAsset, FinalUsedPartTypes, FinalSlugToVariantMap);

			// --- Шаг 4: Применяем меши частей тела победителей ---
			UE_LOG(LogCustomizationComponent, Log, TEXT("Applying final body part meshes..."));
			for (const FName& Slug : FinalActiveSlugs)
			{
				if (const FBodyPartVariant* const* FoundVariant = FinalSlugToVariantMap.Find(Slug))
				{
					// Тип берем из TargetState, который уже обновлен
                    EBodyPartType PartType = TargetState.EquippedBodyPartsItems.FindChecked(Slug);
					UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying mesh for BodyPart: %s (Type: %s)"), *Slug.ToString(), *UEnum::GetValueAsString(PartType));
					CustomizationUtilities::SetBodyPartSkeletalMesh(this, (*FoundVariant)->BodyPartSkeletalMesh, PartType);
				}
			}

			// --- 5: Сбрасываем неиспользуемые меши ---
			// Используем FinalUsedPartTypes, который содержит все активные типы + возможно BodySkin
			ResetUnusedBodyParts(TargetState, FinalUsedPartTypes);

			// --- 6: Обновление Debug Info и вызов InvalidateSkin ---
			// DebugInfo читает из TargetState, который теперь корректен
			DebugInfo.EquippedItems = TargetState.GetItemsList();

			// InvalidateSkin вызывается с TargetState, который теперь корректен
			// Передаем по ссылке для единообразия, хотя InvalidateSkin может не модифицировать его.
			InvalidateSkin(TargetState);

			OnSomatotypeLoaded.Broadcast(SomatotypeDataAsset);
			PendingInvalidationCounter.Pop(); // Pop для лямбды StaticSyncLoadAssetList
		});

	// PendingInvalidationCounter.Pop(); 
                                      
	UE_LOG(LogCustomizationComponent, Log, TEXT("Exiting InvalidateBodyParts (sync load initiated).")); // Changed log message
}

void UCustomizationComponent::ApplyBodySkin(const FCustomizationContextData& TargetState,
                                            const USomatotypeDataAsset* SomatotypeDataAsset,
                                            TSet<EBodyPartType>& FinalUsedPartTypes,
                                            TMap<FName, const FBodyPartVariant*> FinalSlugToVariantMap)
{
	UE_LOG(LogCustomizationComponent, Verbose, TEXT("Applying Body Skin..."));

	FSkinFlagCombination SkinVisibilityFlags;
	for (const auto& Pair : FinalSlugToVariantMap) // Use the map of *actually applied* variants
	{
		if(Pair.Value) // Check validity
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
		if (SkeletalComp && !FinalUsedPartTypes.Contains(PartType)) // Проверяем отсутствие в ИТОГОВОМ наборе
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
	PendingInvalidationCounter.Push(); // Счетчик на всю функцию

	// --- Шаг 0: Определить акторы для удаления ---
    // Сравниваем ключи (слоты) и значения (списки акторов по слага) между Current и Target
    TMap<ECustomizationSlotType, TArray<FName>> SlugsToRemovePerSlot;

    for(const auto& CurrentPair : CurrentCustomizationState.EquippedCustomizationItemActors)
    {
        ECustomizationSlotType CurrentSlot = CurrentPair.Key;
        const FEquippedItemsInSlotInfo& CurrentSlotInfo = CurrentPair.Value;

        const FEquippedItemsInSlotInfo* TargetSlotInfoPtr = TargetState.EquippedCustomizationItemActors.Find(CurrentSlot);

        if (TargetSlotInfoPtr)
        {
            // Diff beetwin Current and Target for slot
            for(const auto& CurrentActorInfo : CurrentSlotInfo.EquippedItemActors)
            {
                bool bFoundInTarget = false;
                for(const auto& TargetActorInfo : TargetSlotInfoPtr->EquippedItemActors)
                {
                    if (CurrentActorInfo.ItemSlug == TargetActorInfo.ItemSlug)
                    {
                        bFoundInTarget = true;
                        break;
                    }
                }
                if (!bFoundInTarget)
                {
                    SlugsToRemovePerSlot.FindOrAdd(CurrentSlot).Add(CurrentActorInfo.ItemSlug);
                }
            }
        }
        else
        {
             for(const auto& CurrentActorInfo : CurrentSlotInfo.EquippedItemActors)
             {
                 SlugsToRemovePerSlot.FindOrAdd(CurrentSlot).Add(CurrentActorInfo.ItemSlug);
             }
        }
    }

	// --- 1: Remove useless actors ---
    for(const auto& RemovePair : SlugsToRemovePerSlot)
    {
        ECustomizationSlotType SlotToRemove = RemovePair.Key;
        const TArray<FName>& SlugsToRemove = RemovePair.Value;

        if(const FEquippedItemsInSlotInfo* CurrentSlotInfoPtr = CurrentCustomizationState.EquippedCustomizationItemActors.Find(SlotToRemove))
        {
            for(const FName& Slug : SlugsToRemove)
            {
                for(const auto& ActorInfo : CurrentSlotInfoPtr->EquippedItemActors)
                {
                    if (ActorInfo.ItemSlug == Slug)
                    {
                        UE_LOG(LogCustomizationComponent, Verbose, TEXT("Removing actors for ItemSlug: %s in Slot: %s"), *Slug.ToString(), *UEnum::GetValueAsString(SlotToRemove));
                        for (const auto& ItemRelatedActor : ActorInfo.ItemRelatedActors)
			            {
				            if (ItemRelatedActor.IsValid())
				            {
					            const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
					            ItemRelatedActor->DetachFromActor(DetachmentTransformRules);
					            ItemRelatedActor->Destroy();
				            }
			            }
                        break;
                    }
                }
            }
        }
    }


	// --- 2: Target&Current: Get slugs for update ---
    TArray<FPrimaryAssetId> AddedOrUpdatedAssetIds;
	TMap<FPrimaryAssetId, FName> AssetIdToSlugMap;
    TMap<FName, ECustomizationSlotType> SlugToSlotMap;

    for(const auto& TargetPair : TargetState.EquippedCustomizationItemActors)
    {
        ECustomizationSlotType TargetSlot = TargetPair.Key;
        const FEquippedItemsInSlotInfo& TargetSlotInfo = TargetPair.Value;

        const FEquippedItemsInSlotInfo* CurrentSlotInfoPtr = CurrentCustomizationState.EquippedCustomizationItemActors.Find(TargetSlot);

        for(const auto& TargetActorInfo : TargetSlotInfo.EquippedItemActors)
        {
            FName TargetSlug = TargetActorInfo.ItemSlug;
            bool bFoundInCurrent = false;
            if (CurrentSlotInfoPtr)
            {
                 for(const auto& CurrentActorInfo : CurrentSlotInfoPtr->EquippedItemActors)
                 {
                      if (CurrentActorInfo.ItemSlug == TargetSlug)
                      {
                          bFoundInCurrent = true;
                          // TODO: if asset same something else not. Check this
                          break;
                      }
                 }
            }

            if (!bFoundInCurrent)
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


	// --- 3: Load ---
    if (AddedOrUpdatedAssetIds.IsEmpty())
    {
        UE_LOG(LogCustomizationComponent, Verbose, TEXT("No actors to add or update."));
		PendingInvalidationCounter.Pop();
    	
        // InvalidateSkin(TargetState); // TODO:: Call again?
		return;
    }

    UE_LOG(LogCustomizationComponent, Log, TEXT("Loading %d assets for actor spawning..."), AddedOrUpdatedAssetIds.Num());
    PendingInvalidationCounter.Push();

	UCustomizationAssetManager::StaticSyncLoadAssetList<UCustomizationDataAsset>(
		AddedOrUpdatedAssetIds, [this, TargetState, AssetIdToSlugMap, SlugToSlotMap](TArray<UCustomizationDataAsset*> LoadedAssets) // Захватываем TargetState и карты
		{
            UE_LOG(LogCustomizationComponent, Log, TEXT("Loaded %d assets for actor spawning."), LoadedAssets.Num());

			// create temporary copy for target state for updated items
            //
            // Update InvalidationContext.Current, cz it's a copy of TargetState already

			for (const auto& CustomizationAsset : LoadedAssets)
			{
                if (!CustomizationAsset) continue;

                const FPrimaryAssetId CustomizationAssetId = CustomizationAsset->GetPrimaryAssetId();
				const FName ItemSlug = AssetIdToSlugMap.FindChecked(CustomizationAssetId);
                const ECustomizationSlotType SlotType = SlugToSlotMap.FindChecked(ItemSlug); // Получаем слот

				UE_LOG(LogCustomizationComponent, Verbose, TEXT("Spawning actors for ItemSlug: %s in Slot: %s"), *ItemSlug.ToString(), *UEnum::GetValueAsString(SlotType));

				TArray<TStrongObjectPtr<AActor>> SpawnedActorPtrs;
                TArray<AActor*> SpawnedActorsRaw;

				const auto& SuitableComplects = CustomizationAsset->CustomizationComplect; // Используем CustomizationComplect напрямую
				if (SuitableComplects.IsEmpty())
				{
					UE_LOG(LogCustomizationComponent, Warning, TEXT("No CustomizationComplect found in asset %s for item %s"), *CustomizationAssetId.ToString(), *ItemSlug.ToString());
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
					if (USkeletalMeshComponent* AttachTarget = Skeletals.FindRef(EBodyPartType::BodySkin))
                    {
					    SpawnedActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Complect.SocketName);
					    SpawnedActor->SetActorRelativeTransform(Complect.RelativeTransform);
                         UE_LOG(LogCustomizationComponent, Verbose, TEXT("Attached spawned actor %s to %s at socket %s"), *SpawnedActor->GetName(), *AttachTarget->GetName(), *Complect.SocketName.ToString());

                        // TODO: material for actor here maybe?
                        // CustomizationUtilities::SetMaterialOnMesh() 
                    }
                    else
                    {
                        UE_LOG(LogCustomizationComponent, Warning, TEXT("Could not find BodySkin mesh component to attach actor %s"), *SpawnedActor->GetName());
                    }
				}

				if (!SpawnedActorsRaw.IsEmpty())
				{
					OnNewCustomizationActorsAttached.Broadcast(SpawnedActorsRaw);

					// 5. Enable Physics 
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
					});
				}

				// --- 6: Update current info ---
                InvalidationContext.Current.ReplaceOrAddSpawnedActors(ItemSlug, SlotType, SpawnedActorPtrs);
                UE_LOG(LogCustomizationComponent, Verbose, TEXT("Updated InvalidationContext.Current with spawned actors for %s"), *ItemSlug.ToString());
			}
			
			DebugInfo.ActorInfo = InvalidationContext.Current.GetActorsList(); // Показываем состояние после спавна
			
			PendingInvalidationCounter.Pop();
		});

	PendingInvalidationCounter.Pop();
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
