#include "UI/VM_Inventory.h"

#include "Components/CustomizationComponent.h"
#include "Components/InventoryComponent.h"

#include "Engine/AssetManager.h"

#include "UI/Inventory/Data/InventoryListItemData.h"

DEFINE_LOG_CATEGORY(LogViewModel);

UVM_Inventory::UVM_Inventory()
{
	bIsLoading = false;
}

TArray<USkinListItemData*> UVM_Inventory::GetSkinsForColorPalette() const
{
	return SkinsForColorPalette;
}

void UVM_Inventory::SetbIsLoading(const bool InbIsLoaded)
{
	bIsLoading = InbIsLoaded;
}

bool UVM_Inventory::IsLoaded() const
{
	return !bIsLoading;
}

TArray<UObject*> UVM_Inventory::GetInventoryItemsAsObjects() const
{
	TArray<UObject*> ObjectList;
	ObjectList.Reserve(InventoryItemsList.Num()); 

	for (const TObjectPtr<UInventoryListItemData>& ItemDataPtr : InventoryItemsList)
	{

		if (ItemDataPtr)
		{
			ObjectList.Add(ItemDataPtr); 
		}
	}
	return ObjectList;
}

EItemSlot UVM_Inventory::GetFilterType() const
{
	return LastFilterType;
}

void UVM_Inventory::Initialize(UInventoryComponent* InInventoryComp, UCustomizationComponent* InCustomizationComp)
{
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::Initialize - Initializing with components."));

	UnbindDelegates();

	InventoryComponent = InInventoryComp;
	CustomizationComponent = InCustomizationComp;

	bool bSuccess = true;

	if (!InventoryComponent.IsValid() || !CustomizationComponent.IsValid())
	{
		UE_LOG(LogViewModel, Error, TEXT("UVM_Inventory::Initialize - Invalid InventoryComponent or CustomizationComponent."));
		bSuccess = false;
	}

	InventoryComponent->OnInventoryChanged.AddDynamic(this, &UVM_Inventory::HandleInventoryDeltaUpdate);
	CustomizationComponent->OnEquippedItemsChanged.AddDynamic(this, &UVM_Inventory::HandleEquippedItemsUpdate);

	if (bSuccess)
	{
		UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::Initialize - Components seem valid, performing initial data refresh."));
		RefreshAllViewModelData();
	}
	else
	{
		UE_LOG(LogViewModel, Error, TEXT("UVM_Inventory::Initialize - Initialization failed due to invalid components. Clearing VM data."));
		SetInventoryItemsList({});
		SetEquippedItemsMap({});
		SetIsLoading(false);
		LastKnownOwnedItems.Empty();

		// LastKnownCustomizationState = FCustomizationContextData(); // ??
	}
}

bool UVM_Inventory::RequestEquipItem(const FName& ItemSlugToEquip)
{
	if (CustomizationComponent.IsValid())
	{
		if (ItemSlugToEquip != NAME_None)
		{
			UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::RequestEquipItem - Requesting to equip item: %s"), *ItemSlugToEquip.ToString());
			CustomizationComponent->EquipItem(ItemSlugToEquip);
			return true;
		}
	}
	return false;
}

bool UVM_Inventory::RequestUnequipItem(const FName& ItemSlugToUnequip)
{
	if (CustomizationComponent.IsValid())
	{
		if (ItemSlugToUnequip != NAME_None)
		{
			UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::RequestUnequipItem - Requesting to unequip item: %s"), *ItemSlugToUnequip.ToString());
			CustomizationComponent->UnequipItem(ItemSlugToUnequip);
			return true;
		}
		else
		{
			UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::RequestUnequipItem - Received NAME_None slug. Ignoring request."));
		}
	}
	else
	{
		UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::RequestUnequipItem - CustomizationComponent is invalid. Cannot unequip item."));
	}
	return false;
}

void UVM_Inventory::OnEntryItemClicked(const FName& InItemSlug)
{
	if (IsItemSlugEquipped(InItemSlug))
	{
		RequestUnequipItem(InItemSlug);
	}
	else
	{
		RequestEquipItem(InItemSlug);
	}
}

void UVM_Inventory::RequestUnequipSlot(ECustomizationSlotType SlotToUnequip)
{
	if (CustomizationComponent.IsValid())
	{
		if (SlotToUnequip != ECustomizationSlotType::Unknown)
		{
			UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::RequestUnequipSlot - Requesting to unequip slot: %s"), *UEnum::GetValueAsString(SlotToUnequip));
			
			CustomizationComponent->RequestUnequipSlot(SlotToUnequip);
			// TODO:: Implement in  CustomizationComponent. Maybe it will be needed
		}
		else
		{
			UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::RequestUnequipSlot - Received Unknown slot type. Ignoring request."));
		}
	}
	else
	{
		UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::RequestUnequipSlot - CustomizationComponent is invalid. Cannot unequip slot."));
	}
}

void UVM_Inventory::RequestDropItem(FPrimaryAssetId ItemIdToDrop, int32 CountToDrop)
{
	if (InventoryComponent.IsValid())
	{
		if (!ItemIdToDrop.IsValid() || CountToDrop <= 0)
		{
			UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::RequestDropItem - Invalid parameters provided (ID: %s, Count: %d). Ignoring request."), *ItemIdToDrop.ToString(), CountToDrop);
			return;
		}

		UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::RequestDropItem - Requesting to drop item ID: %s, Count: %d"), *ItemIdToDrop.ToString(), CountToDrop);


		UE_LOG(LogViewModel, Error, TEXT("UVM_Inventory::RequestDropItem - Functionality to remove item by ID is NOT IMPLEMENTED in UInventoryComponent!"));
		//InventoryComponent->AddItem(ItemIdToDrop, CountToDrop);
	}
	else
	{
		UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::RequestDropItem - InventoryComponent is invalid. Cannot drop item."));
	}
}

void UVM_Inventory::SetInventoryItemsList(const TArray<TObjectPtr<UInventoryListItemData>>& InNewList)
{
	UE_MVVM_SET_PROPERTY_VALUE(InventoryItemsList, InNewList);
}

void UVM_Inventory::SetEquippedItemsMap(const TMap<EItemSlot, FInventoryEquippedItemData>& NewMap)
{
	bool bMapsAreEqual = true;
	if (EquippedItemsMap.Num() != NewMap.Num())
	{
		bMapsAreEqual = false;
	}
	else
	{
		for (const auto& Pair : NewMap)
		{
			const EItemSlot& Key = Pair.Key;
			const FInventoryEquippedItemData& NewValue = Pair.Value;
			const FInventoryEquippedItemData* CurrentValuePtr = EquippedItemsMap.Find(Key);
			if (!CurrentValuePtr || !(*CurrentValuePtr == NewValue))
			{
				bMapsAreEqual = false;
				break;
			}
		}
	}

	if (!bMapsAreEqual)
	{
		UE_LOG(LogViewModel, Log, TEXT("SetEquippedItemsMap: Maps are different, updating and broadcasting."));
		//EquippedItemsMap = NewMap;
		
		UE_MVVM_SET_PROPERTY_VALUE(EquippedItemsMap, NewMap);
		BroadcastEquippedItemChanges();
	}
	else
	{
		UE_LOG(LogViewModel, Verbose, TEXT("SetEquippedItemsMap: Maps are identical, skipping update."));
	}
}

void UVM_Inventory::SetItemsCount(const int32 NewCount)
{
	// TODO:: ?
	UE_MVVM_SET_PROPERTY_VALUE(ItemsCount, NewCount);
}

void UVM_Inventory::HandleEquippedItemsUpdate(const FCustomizationContextData& NewState)
{
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::HandleEquippedItemsUpdate - Received equipment update signal. Applying targeted slot update."));

	// --- Step 1: Calculate new map ---
	TMap<EItemSlot, FInventoryEquippedItemData> NewCalculatedEquippedMap;
	TSet<FName> AllEquippedSlugsForState;

	auto FindMetaAssetBySlug = [this](FName Slug) -> UItemMetaAsset* {
		for (const auto& CachePair : LoadedMetaCache)
		{
			if (CachePair.Value )
			{
				// UE_LOG(LogViewModel, Warning, TEXT("  Cached MetaAsset FName: %s"), *CachePair.Value->GetFName().ToString());
				if (CachePair.Value->GetFName() == Slug)
				{
					return CachePair.Value;
				}
			}
		}
		return nullptr;
	};
	
	auto ProcessBaseEquippedItemSlugForMap =
		[&](FName ItemSlug, TMap<EItemSlot, FInventoryEquippedItemData>& TargetMap)
	{
			if (ItemSlug == NAME_None) return;

			UItemMetaAsset* MetaAsset = FindMetaAssetBySlug(ItemSlug);
			if (!MetaAsset) return;
			
			if (MetaAsset->ItemType == EItemType::Skin) return;

			EItemSlot CurrentItemSlot = MetaAsset->ItemSlot;
			if (CurrentItemSlot == EItemSlot::None) return;

			FInventoryEquippedItemData EquippedData;
			EquippedData.ItemName = MetaAsset->Name;
			EquippedData.Icon = MetaAsset->Icon;
			EquippedData.Tier = MetaAsset->ItemTier;
			EquippedData.ItemSlug = MetaAsset->GetPrimaryAssetId().PrimaryAssetName;
			// AppliedSkinSlug == NAME_None by default
			
			TargetMap.Add(CurrentItemSlot, EquippedData);
	};

	// Collect ALL equipped slugs first, including materials, for 'IsEquipped' checks.
	for (const auto& Pair : NewState.EquippedBodyPartsItems) { AllEquippedSlugsForState.Add(Pair.Key); }
	for (const auto& Pair : NewState.EquippedMaterialsMap) { AllEquippedSlugsForState.Add(Pair.Key); }
	for (const auto& SlotPair : NewState.EquippedCustomizationItemActors)
	{
		for (const FEquippedItemActorsInfo& ActorInfo : SlotPair.Value.EquippedItemActors)
		{
			AllEquippedSlugsForState.Add(ActorInfo.ItemSlug);
		}
	}

	// Now, populate the map for UI slots, IGNORING materials/skins.
	for (const auto& Pair : NewState.EquippedBodyPartsItems) { ProcessBaseEquippedItemSlugForMap(Pair.Key, NewCalculatedEquippedMap); }
	for (const auto& SlotPair : NewState.EquippedCustomizationItemActors)
	{
		for (const FEquippedItemActorsInfo& ActorInfo : SlotPair.Value.EquippedItemActors)
		{
			ProcessBaseEquippedItemSlugForMap(ActorInfo.ItemSlug, NewCalculatedEquippedMap);
		}
	}
    
    // Step 2. Update info about skins ---
    for (const auto& Pair : NewState.EquippedMaterialsMap)
    {
        const FName& SkinSlug = Pair.Key;
        UItemMetaAsset* SkinMetaAsset = FindMetaAssetBySlug(SkinSlug);

        if (SkinMetaAsset && SkinMetaAsset->ItemType == EItemType::Skin)
        {
            if (FInventoryEquippedItemData* SlotData = NewCalculatedEquippedMap.Find(SkinMetaAsset->ItemSlot))
            {
                SlotData->AppliedSkinSlug = SkinSlug;
            }
        }
    }


	// --- Step 3: Get corrupted types EItemType ---
	TSet<EItemSlot> AffectedSlots;
	
	// diff NewCalculatedEquippedMap and this->EquippedItemsMap
	TArray<EItemSlot> CurrentKeys;
	TArray<EItemSlot> NewKeys;
	EquippedItemsMap.GenerateKeyArray(CurrentKeys);
	NewCalculatedEquippedMap.GenerateKeyArray(NewKeys);

	TSet<EItemSlot> CurrentKeysSet(CurrentKeys);
	TSet<EItemSlot> NewKeysSet(NewKeys);

	AffectedSlots.Append(NewKeysSet.Difference(CurrentKeysSet));
	AffectedSlots.Append(CurrentKeysSet.Difference(NewKeysSet));

	for (const EItemSlot& Key : CurrentKeysSet.Intersect(NewKeysSet))
	{
		// Используем наш перегруженный оператор== для FInventoryEquippedItemData
		if (!(EquippedItemsMap.FindChecked(Key) == NewCalculatedEquippedMap.FindChecked(Key)))
		{
			AffectedSlots.Add(Key);
		}
	}
	
	if (AffectedSlots.IsEmpty() && EquippedItemsMap.Num() == NewCalculatedEquippedMap.Num())
	{
		UE_LOG(LogViewModel, Verbose, TEXT("HandleEquippedItemsUpdate: No changes detected in EquippedItemsMap. Skipping broadcasts for slots."));
	}

	// --- Step 4: update map in ViewModel ---
	SetEquippedItemsMap(NewCalculatedEquippedMap); 

	// Step 5: Broadcast to only changed types
	if (!AffectedSlots.IsEmpty())
	{
		UE_LOG(LogViewModel, Log, TEXT("HandleEquippedItemsUpdate: Broadcasting changes for affected slots: %d types."), AffectedSlots.Num());
		for (const EItemSlot AffectedSlot : AffectedSlots)
		{
			BroadcastGetterForType(AffectedSlot);
		}
	}

	// --- Step 6: update bIsEquipped in InventoryItemsList ---
	for (TObjectPtr<UInventoryListItemData>& ItemDataPtr  : InventoryItemsList)
	{
		if (!ItemDataPtr) continue; 

		bool bShouldBeEquippedNow = AllEquippedSlugsForState.Contains(ItemDataPtr->ItemSlug);
		if (ItemDataPtr->GetIsEquipped() != bShouldBeEquippedNow)
		{
			ItemDataPtr->SetIsEquipped(bShouldBeEquippedNow);
			UE_LOG(LogViewModel, Verbose, TEXT("... Updating bIsEquipped for %s"), *ItemDataPtr->ItemSlug.ToString());
		}
	}
	
	// --- Step 7: broadcast to inventory list al well. Maybe not needed but ok for now TODO:: check ---
	//if (bInventoryListNeedsUpdate)
	//{
	//	UE_LOG(LogViewModel, Log, TEXT("HandleEquippedItemsUpdate: Broadcasting change for InventoryItemsList due to bIsEquipped updates."));
	//	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(InventoryItemsList);
	//}
	
	LastKnownCustomizationState = NewState;
}

void UVM_Inventory::HandleInventoryDeltaUpdate(UItemMetaAsset* InItem, const int32 CountDelta)
{
	if (!InItem)
	{
		UE_LOG(LogViewModel, Warning, TEXT("HandleInventoryDeltaUpdate - Received null MetaAsset. Falling back to full refresh."));
		RefreshAllViewModelData();
		return;
	}

	FPrimaryAssetId ItemId = InItem->GetPrimaryAssetId();
	if (!ItemId.IsValid())
	{
		UE_LOG(LogViewModel, Error, TEXT("HandleInventoryDeltaUpdate - Received MetaAsset '%s' with invalid PrimaryAssetId. Cannot process update."), *InItem->GetName());
		return;
	}

	if (CountDelta == 0)
	{
		UE_LOG(LogViewModel, Verbose, TEXT("HandleInventoryDeltaUpdate - Received update for item %s with zero delta. No action taken."), *ItemId.ToString());
		return;
	}

	UE_LOG(LogViewModel, Log, TEXT("HandleInventoryDeltaUpdate - Processing update for Item: %s, Delta: %d"), *ItemId.ToString(), CountDelta);
	
	if (!LoadedMetaCache.Contains(ItemId))
	{
		LoadedMetaCache.Add(ItemId, InItem);
		UE_LOG(LogViewModel, Verbose, TEXT("HandleInventoryDeltaUpdate - Cached metadata for '%s' directly from delegate."), *ItemId.ToString());
	}
	// 1.
	int32 FoundIndex = InventoryItemsList.IndexOfByPredicate([ItemId](const TObjectPtr<UInventoryListItemData>& Data)
	{
		return Data && Data->ItemId == ItemId;
	});

	bool bListNeedsBroadcast = false;

	// 2. Update data due to diff
	if (FoundIndex != INDEX_NONE)
	{
		TObjectPtr<UInventoryListItemData> ExistingItem = InventoryItemsList[FoundIndex];
		if (!ExistingItem)
		{
			return;
		}

		
		int32 OldCount = ExistingItem->Count;
		int32 NewCount = OldCount + CountDelta;

		if (NewCount > 0)
		{
			UE_LOG(LogViewModel, Verbose, TEXT("HandleInventoryDeltaUpdate - Updating count for existing item %s from %d to %d (Delta: %d)"),
				*ItemId.ToString(), OldCount, NewCount, CountDelta);

			ExistingItem->Count = NewCount;
			bListNeedsBroadcast = true;
			// TODO:: 
			// if UInventoryListItemData is Observable call:
			// UE_MVVM_SET_PROPERTY_VALUE(ExistingItem->Count, NewCount);
		}
		else
		{
			UE_LOG(LogViewModel, Verbose, TEXT("HandleInventoryDeltaUpdate - Removing item %s (OldCount: %d, Delta: %d, NewCount: %d)"),
				*ItemId.ToString(), OldCount, CountDelta, NewCount);
			InventoryItemsList.RemoveAt(FoundIndex);
			bListNeedsBroadcast = true;
		}
	}
	else
	{
		if (CountDelta > 0)
		{
			UE_LOG(LogViewModel, Verbose, TEXT("Adding new item %s with count %d"), *ItemId.ToString(), CountDelta);
			UInventoryListItemData* NewItemObject = NewObject<UInventoryListItemData>(this);

			NewItemObject->SetEventSubscriptionFunctions(
				[ViewModelWeakPtr = MakeWeakObjectPtr(this)](UObject* SubscriberWidget, TFunction<void(EItemSlot)> WidgetHandler) -> FDelegateHandle
				{
					// ensureAlwaysMsgf(ViewModelWeakPtr.IsValid(), TEXT("ViewModelWeakPtr is invalid inside Subscribe lambda creation!"));
					if (ViewModelWeakPtr.IsValid() && SubscriberWidget && WidgetHandler)
					{
						UVM_Inventory* VM = ViewModelWeakPtr.Get();
						FDelegateHandle Handle = VM->OnFilterMethodChanged.AddWeakLambda(SubscriberWidget,
						                                                                 [WidgetHandler](EItemSlot FilterSlot)
						                                                                 {
							                                                                 if (WidgetHandler) WidgetHandler(FilterSlot);
						                                                                 }
						);

						if (WidgetHandler)
						{
							WidgetHandler(VM->GetFilterType());
						}
						return Handle;
					}
					return FDelegateHandle();
				},
				[ViewModelWeakPtr = MakeWeakObjectPtr(this)](FDelegateHandle Handle)
				{
					ensureAlwaysMsgf(ViewModelWeakPtr.IsValid(), TEXT("ViewModelWeakPtr is invalid inside Subscribe lambda creation!"));
					if (ViewModelWeakPtr.IsValid() && Handle.IsValid())
					{
						ViewModelWeakPtr->OnFilterMethodChanged.Remove(Handle);
					}
				}
			);
			
			NewItemObject->InitializeFromMeta(
				InItem,
				CountDelta,
				IsItemSlugEquipped(InItem->GetFName())
			);
			InventoryItemsList.Add(NewItemObject); 
			bListNeedsBroadcast = true; 
		}
	}

	// 3. call UI about changes
	if (bListNeedsBroadcast)
	{
		// broadcast all list but data is child of UObject and that's why ListView
		// will not call every entry but calls correctly only changes
		// Partically update tested

		UE_LOG(LogViewModel, Log, TEXT("Broadcasting InventoryItemsList change."));
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(InventoryItemsList);
		SetItemsCount(InventoryItemsList.Num());
	}
}

void UVM_Inventory::RequestMetaDataAndExecute(const TSet<FPrimaryAssetId>& MetaAssetIdsToEnsure, FOnMetaDataRequestCompleted OnCompleteDelegate)
{
	UE_LOG(LogViewModel, Log, TEXT("RequestMetaDataAndExecute - Requesting %d IDs."), MetaAssetIdsToEnsure.Num());
	
	UAssetManager& AssetManager = UAssetManager::Get();
	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();

	TSet<FPrimaryAssetId> IdsToActuallyLoad;
	TArray<FSoftObjectPath> PathsToLoad;

	// 1. 
	for (const FPrimaryAssetId& ItemId : MetaAssetIdsToEnsure)
	{
		if (ItemId.IsValid() && !LoadedMetaCache.Contains(ItemId))
		{
			bool bAlreadyLoading = false;
			for (const auto& ExistingRequest : ActiveMetaRequests)
			{
				if (ExistingRequest.LoadHandle.IsValid() && !ExistingRequest.LoadHandle->HasLoadCompleted() && ExistingRequest.RequestedIds.Contains(ItemId))
				{
					bAlreadyLoading = true;
					UE_LOG(LogViewModel, Verbose, TEXT("RequestMetaDataAndExecute - ID %s is already loading in another request (Index %d). Skipping add here."), *ItemId.ToString(), ActiveMetaRequests.IndexOfByPredicate([&](const FPendingMetaRequest& R){ return &R == &ExistingRequest; }));
					break;
				}
			}

			if (!bAlreadyLoading)
			{
				FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ItemId);
				if (AssetPath.IsValid())
				{
					IdsToActuallyLoad.Add(ItemId);
					PathsToLoad.Add(AssetPath);
					UE_LOG(LogViewModel, Verbose, TEXT("RequestMetaDataAndExecute - ID %s needs loading."), *ItemId.ToString());
				}
				else
				{
					UE_LOG(LogViewModel, Warning, TEXT("RequestMetaDataAndExecute - Could not get valid SoftObjectPath for %s."), *ItemId.ToString());
					LoadedMetaCache.Add(ItemId, nullptr);
				}
			}
		}
	}

	// 2. If nothing needs to be loaded -> perform a callback immediately
	// Important: We check not only Paths To Load, but also MetaAssetIdsToEnsure.
	// The callback must be executed if *all* the requested assets are either already in the cache,
	// or they are already being loaded with another request (we will wait for its completion and callback).
	// A simple PathsToLoad check is not enough.
	// More correctly: if PathsToLoad is empty, it means there is nothing to load for request.
	if (PathsToLoad.IsEmpty())
	{
		bool bWaitingForOtherLoads = false;
		for (const FPrimaryAssetId& ItemId : MetaAssetIdsToEnsure)
		{
			if(ItemId.IsValid() && !LoadedMetaCache.Contains(ItemId))
			{
				for (const auto& ExistingRequest : ActiveMetaRequests)
				{
					if (ExistingRequest.LoadHandle.IsValid() && !ExistingRequest.LoadHandle->HasLoadCompleted() && ExistingRequest.RequestedIds.Contains(ItemId))
					{
						bWaitingForOtherLoads = true;
						UE_LOG(LogViewModel, Log, TEXT("RequestMetaDataAndExecute - ID %s is needed, but loading in another request. This request will wait."), *ItemId.ToString());
						// TODO: More complex logic is needed to "subscribe" to the completion of someone else's request,
						// otherwise, the callback of this request may never be called if another request does not trigger it.
						// // So far, if PathsToLoad is empty, we just perform the callback immediately.
						// This means that if the asset is loaded with another request, we will not wait for it here.
						// The logic of the caller must be prepared for the fact that the asset may not be in the cache yet.
						break;
					}
				}
			}
			if (bWaitingForOtherLoads) break;
		}


		if (!bWaitingForOtherLoads)
		{
			UE_LOG(LogViewModel, Log, TEXT("RequestMetaDataAndExecute - All requested IDs (%d) are cached, invalid, or loading elsewhere (not waiting). Executing callback immediately."), MetaAssetIdsToEnsure.Num());
			OnCompleteDelegate.ExecuteIfBound();
			CheckLoadingState();
			return;
		}
		else
		{
			UE_LOG(LogViewModel, Log, TEXT("RequestMetaDataAndExecute - Some IDs are loading in other requests. This request (%d IDs to load) will proceed without loading them now."), PathsToLoad.Num());
		}

	}

	// 3. 
	SetIsLoading(true);

	int32 RequestIndex = ActiveMetaRequests.Emplace(IdsToActuallyLoad, OnCompleteDelegate);
	FPendingMetaRequest& NewRequest = ActiveMetaRequests[RequestIndex];

	UE_LOG(LogViewModel, Log, TEXT("RequestMetaDataAndExecute - Starting async load for %d assets for request index %d."), PathsToLoad.Num(), RequestIndex);
	
	FStreamableDelegate LoadCompletedDelegate = FStreamableDelegate::CreateUObject(this, &UVM_Inventory::OnMetaDataRequestCompleted, RequestIndex);

	NewRequest.LoadHandle = StreamableManager.RequestAsyncLoad(
		PathsToLoad,
		LoadCompletedDelegate,
		FStreamableManager::DefaultAsyncLoadPriority,
		false, false, TEXT("InventoryVMMetaSpecificLoad")
	);

	if (!NewRequest.LoadHandle.IsValid())
	{
		UE_LOG(LogViewModel, Error, TEXT("RequestMetaDataAndExecute - Failed to start async load request index %d!"), RequestIndex);
		ActiveMetaRequests.RemoveAt(RequestIndex); 
		CheckLoadingState();
		OnCompleteDelegate.ExecuteIfBound();
	}
}

void UVM_Inventory::CheckLoadingState()
{
}

void UVM_Inventory::OnMetaDataRequestCompleted(int32 RequestIndex)
{
	if (!ActiveMetaRequests.IsValidIndex(RequestIndex))
	{
		UE_LOG(LogViewModel, Error, TEXT("OnMetaDataRequestCompleted - Invalid RequestIndex %d received!"), RequestIndex);
		return;
	}

	// copy data before deleting it, in case the delegate calls a new request
	FPendingMetaRequest CompletedRequest = ActiveMetaRequests[RequestIndex];
	TSharedPtr<FStreamableHandle> Handle = CompletedRequest.LoadHandle;
	
	ActiveMetaRequests.RemoveAt(RequestIndex);

	UE_LOG(LogViewModel, Log, TEXT("OnMetaDataRequestCompleted - Async load completed for request index %d. Processing %d expected IDs."), RequestIndex, CompletedRequest.RequestedIds.Num());
	
	UAssetManager& AssetManager = UAssetManager::Get();
	int32 SuccessfullyLoadedCount = 0;
	for (const FPrimaryAssetId& ItemId : CompletedRequest.RequestedIds)
	{
		if (!LoadedMetaCache.Contains(ItemId))
		{
			UObject* LoadedObject = AssetManager.GetPrimaryAssetObject(ItemId);
			UItemMetaAsset* MetaAsset = Cast<UItemMetaAsset>(LoadedObject);
			if (MetaAsset)
			{
				LoadedMetaCache.Add(ItemId, MetaAsset);
				SuccessfullyLoadedCount++;
				UE_LOG(LogViewModel, Verbose, TEXT("OnMetaDataRequestCompleted - Cached metadata for: %s"), *ItemId.ToString());
			}
			else
			{
				bool bWasLoadedByHandle = false;
				if (Handle.IsValid() && Handle->HasLoadCompleted()) {
					TArray<UObject*> LoadedObjects;
					Handle->GetLoadedAssets(LoadedObjects);
					for(UObject* Obj : LoadedObjects) {
						if(Obj && Obj->GetPrimaryAssetId() == ItemId) {
							bWasLoadedByHandle = true;
							break;
						}
					}
				}

				if(bWasLoadedByHandle) {
				    UE_LOG(LogViewModel, Warning, TEXT("OnMetaDataRequestCompleted - Failed to cast loaded object to UItemMetaAsset for ID: %s. Object: %s. Caching nullptr."), *ItemId.ToString(), *GetNameSafe(LoadedObject));
				} else {
					UE_LOG(LogViewModel, Error, TEXT("OnMetaDataRequestCompleted - Failed to load metadata for ID: %s (not found in completed handle or AssetManager). Caching nullptr."), *ItemId.ToString());
				}
				LoadedMetaCache.Add(ItemId, nullptr);
			}
		}
		else
		{
			SuccessfullyLoadedCount++;
			UE_LOG(LogViewModel, Verbose, TEXT("OnMetaDataRequestCompleted - Metadata for %s was already in cache."), *ItemId.ToString());
		}
	}
	UE_LOG(LogViewModel, Log, TEXT("OnMetaDataRequestCompleted - Request index %d ensured %d assets are cached."), RequestIndex, SuccessfullyLoadedCount);
	
	UE_LOG(LogViewModel, Log, TEXT("OnMetaDataRequestCompleted - Executing completion delegate for request index %d."), RequestIndex);
	CompletedRequest.CompletionDelegate.ExecuteIfBound();
	
	CheckLoadingState();
}

void UVM_Inventory::CancelAllMetaRequests()
{
	UE_LOG(LogViewModel, Log, TEXT("CancelAllMetaRequests - Canceling %d active requests."), ActiveMetaRequests.Num());
	for (FPendingMetaRequest& Request : ActiveMetaRequests)
	{
		if (Request.LoadHandle.IsValid())
		{
			Request.LoadHandle->CancelHandle();
		}
		Request.CompletionDelegate.Unbind();
	}
	ActiveMetaRequests.Empty();
	if (bIsLoading)
	{
		SetIsLoading(false);
	}
}

bool UVM_Inventory::IsItemSlugEquipped(const FName& ItemSlug)
{
	if (ItemSlug == NAME_None) return false;

	for (const auto& Pair : EquippedItemsMap)
	{
		if (Pair.Value.ItemSlug == ItemSlug || Pair.Value.AppliedSkinSlug == ItemSlug)
		{
			return true;
		}
	}
	return false;
}

void UVM_Inventory::RefreshAllViewModelData()
{
	if (!InventoryComponent.IsValid() || !CustomizationComponent.IsValid())
	{
		UE_LOG(LogViewModel, Warning, TEXT("RefreshAllViewModelData - Cannot refresh, components invalid. Clearing VM data."));
		SetInventoryItemsList({});
		SetEquippedItemsMap({});
		SetIsLoading(false);
		LastKnownOwnedItems.Empty();
		LoadedMetaCache.Empty();
		CancelAllMetaRequests();
		return;
	}
	
	const TMap<FPrimaryAssetId, int32> CurrentOwnedItems = InventoryComponent->GetOwnedItems();
	const FCustomizationContextData CurrentCustomizationState = CustomizationComponent->GetCurrentCustomizationState();
	
	bool bInventoryChanged = !(LastKnownOwnedItems == CurrentOwnedItems);
	bool bEquipmentChanged = !(LastKnownCustomizationState == CurrentCustomizationState);

	if (!bInventoryChanged && !bEquipmentChanged && !bIsLoading)
	{
		//UE_LOG(LogViewModel, Verbose, TEXT("RefreshAllViewModelData - No changes detected in inventory or equipment. Skipping refresh."));
		return;
	}
	
	LastKnownOwnedItems = CurrentOwnedItems;
	LastKnownCustomizationState = CurrentCustomizationState;
	
	TSet<FPrimaryAssetId> RequiredMetaAssetIdsSet;
	for (const auto& Pair : LastKnownOwnedItems)
	{
		if (Pair.Key.IsValid())
		{
			RequiredMetaAssetIdsSet.Add(Pair.Key);
		}
		///else
		//{
		//	UE_LOG(LogViewModel, Warning, TEXT("RefreshAllViewModelData - Found invalid PrimaryAssetId in OwnedItems."));
		//}
	}

	if (RequiredMetaAssetIdsSet.IsEmpty())
	{
		UE_LOG(LogViewModel, Log, TEXT("RefreshAllViewModelData - No items in inventory. Populating empty lists."));
		
		PopulateViewModelProperties();
		CheckLoadingState();
		return;
	}
	
	FOnMetaDataRequestCompleted PopulateCallback = FOnMetaDataRequestCompleted::CreateUObject(this, &UVM_Inventory::PopulateViewModelProperties);

	UE_LOG(LogViewModel, Log, TEXT("RefreshAllViewModelData - Requesting metadata ensure for %d IDs. PopulateViewModelProperties will be called upon completion."), RequiredMetaAssetIdsSet.Num());
	RequestMetaDataAndExecute(RequiredMetaAssetIdsSet, PopulateCallback);
}

void UVM_Inventory::LoadRequiredMetaData(const TArray<FPrimaryAssetId>& MetaAssetIdsToLoad)
{
	UE_LOG(LogViewModel, Log, TEXT("LoadRequiredMetaData - Entry. Pending IDs: %d"), PendingMetaLoadIds.Num());
	for (const auto& PId : PendingMetaLoadIds)
	{
		UE_LOG(LogViewModel, Log, TEXT("  - Pending: %s"), *PId.ToString());
	}
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::LoadRequiredMetaData - Requested to load metadata for %d IDs."), MetaAssetIdsToLoad.Num());

	if (!CurrentMetaLoadHandle)
	{
		PendingMetaLoadIds.Reset();
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();

	if (CurrentMetaLoadHandle.IsValid() && !CurrentMetaLoadHandle->HasLoadCompleted())
	{
		UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::LoadRequiredMetaData - Canceling previous incomplete load request."));
		CurrentMetaLoadHandle->CancelHandle();
	}
	CurrentMetaLoadHandle.Reset();
	
	TSet<FPrimaryAssetId> IdsToActuallyLoadSet;

	for (const FPrimaryAssetId& ItemId : MetaAssetIdsToLoad)
	{
		bool bInCache = LoadedMetaCache.Contains(ItemId);
		bool bIsPending = PendingMetaLoadIds.Contains(ItemId);
		UE_LOG(LogViewModel, Verbose, TEXT("Checking ID %s: InCache=%s, IsPending=%s"), *ItemId.ToString(), bInCache ? TEXT("Yes") : TEXT("No"), bIsPending ? TEXT("Yes") : TEXT("No"));
		if (!bInCache && !bIsPending)
		{
			IdsToActuallyLoadSet.Add(ItemId);
			UE_LOG(LogViewModel, Verbose, TEXT(" --> Added to IdsToActuallyLoad"));
		}

		if (!ItemId.IsValid())
		{
			UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::LoadRequiredMetaData - Skipping invalid ItemId requested for load: %s"), *ItemId.ToString());
			continue;
		}
		if (!LoadedMetaCache.Contains(ItemId) && !PendingMetaLoadIds.Contains(ItemId))
		{
			IdsToActuallyLoadSet.Add(ItemId);
		}
	}

	TArray<FPrimaryAssetId> IdsToActuallyLoad = IdsToActuallyLoadSet.Array();

	// .If everything is ok -- we can just update VM
	if (IdsToActuallyLoad.IsEmpty())
	{
		UE_LOG(LogViewModel, Log, TEXT("LoadRequiredMetaData: Nothing new to load or all pending. Calling Populate."));
		TriggerPopulateViewModelProperties();

		if (bIsLoading && PendingMetaLoadIds.IsEmpty())
		{
			SetIsLoading(false);
		}
		return;
	}
	
	TArray<FSoftObjectPath> PathsToLoad;
	PathsToLoad.Reserve(IdsToActuallyLoad.Num());

	UE_LOG(LogViewModel, Verbose, TEXT("UVM_Inventory::LoadRequiredMetaData - Converting %d PrimaryAssetIds to SoftObjectPaths..."), IdsToActuallyLoad.Num());
	for (const FPrimaryAssetId& ItemId : IdsToActuallyLoad)
	{
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ItemId);
		if (AssetPath.IsValid())
		{
			PathsToLoad.Add(AssetPath);
			UE_LOG(LogViewModel, Verbose, TEXT("  - Converted %s -> %s"), *ItemId.ToString(), *AssetPath.ToString());
		}
		else
		{
			UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::LoadRequiredMetaData - Could not get valid SoftObjectPath for PrimaryAssetId: %s. Skipping."), *ItemId.ToString());
		}
	}

	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::LoadRequiredMetaData - Path conversion finished. Found %d valid paths."), PathsToLoad.Num());
	if (PathsToLoad.IsEmpty())
	{
		UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::LoadRequiredMetaData - No valid SoftObjectPaths were generated from the requested IDs. Cannot start load."));

		for (const FPrimaryAssetId& FailedId : IdsToActuallyLoad)
		{
			PendingMetaLoadIds.Remove(FailedId);
		}
		if (PendingMetaLoadIds.IsEmpty())
		{
			SetIsLoading(false);
		}
		UE_LOG(LogViewModel, Log, TEXT("Calling PopulateViewModelProperties : %hs"), __FUNCTION__);
		TriggerPopulateViewModelProperties();
		return;
	}

	SetIsLoading(true);
	PendingMetaLoadIds.Append(IdsToActuallyLoad);

	FStreamableDelegate OnMetaDataLoadedDelegate = FStreamableDelegate::CreateUObject(this, &UVM_Inventory::OnMetaDataLoaded);

	CurrentMetaLoadHandle = StreamableManager.RequestAsyncLoad(
		PathsToLoad,
		OnMetaDataLoadedDelegate,
		FStreamableManager::DefaultAsyncLoadPriority,
		false, false, TEXT("InventoryVMMetaLoad")
	);

	if (!CurrentMetaLoadHandle.IsValid())
	{
		//UE_LOG(LogViewModel, Error, TEXT("UVM_Inventory::LoadRequiredMetaData - Failed to start async load request via StreamableManager!"));
		for (const FPrimaryAssetId& FailedId : IdsToActuallyLoad) { PendingMetaLoadIds.Remove(FailedId); }
		if (PendingMetaLoadIds.IsEmpty()) { SetIsLoading(false); }
		//UE_LOG(LogViewModel, Log, TEXT("Calling PopulateViewModelProperties : %hs"), __FUNCTION__);
		TriggerPopulateViewModelProperties();
	}
	else
	{
		//UE_LOG(LogViewModel, Warning, TEXT(">>>> LoadRequiredMetaData - Adding to Pending IDs (using AddUnique):"));
		int32 AddedCount = 0;
		for (const FPrimaryAssetId& AId : IdsToActuallyLoad)
		{
			if (PendingMetaLoadIds.AddUnique(AId) != INDEX_NONE)
			{
				//UE_LOG(LogViewModel, Warning, TEXT("  - Added Unique: %s"), *AId.ToString());
				AddedCount++;
			}
			//lse
			//{
			//	UE_LOG(LogViewModel, Warning, TEXT("  - Already Pending: %s"), *AId.ToString());
			//}
		}
		UE_LOG(LogViewModel, Log, TEXT("... Added %d new unique IDs to pending list. Total pending: %d"), AddedCount, PendingMetaLoadIds.Num());
	}
}

void UVM_Inventory::OnMetaDataLoaded()
{
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::OnMetaDataLoaded - Async load completed callback received."));
	
	// this list can change if new request was started before the old one finished,
	// but since we cancel the old handle, this situation is less likely.
	
	// If the code is more complex and can have multiple active requests (which CurrentMetaLoadHandle does not allow),
	// a more complex tracking logic will be needed.

	// TODO:: think about 

	TArray<FPrimaryAssetId> JustCompletedLoadIds = PendingMetaLoadIds;
	PendingMetaLoadIds.Empty();
	CurrentMetaLoadHandle.Reset();

	UAssetManager& AssetManager = UAssetManager::Get();
	int32 SuccessfullyLoadedCount = 0;
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::OnMetaDataLoaded - Processing %d assets that were pending."), JustCompletedLoadIds.Num());
		
	for (const FPrimaryAssetId& ItemId : JustCompletedLoadIds)
	{
		if (!ItemId.IsValid()) continue;
		if (LoadedMetaCache.Contains(ItemId))
		{
			continue;
		}

		UObject* LoadedObject = AssetManager.GetPrimaryAssetObject(ItemId);
		UItemMetaAsset* MetaAsset = Cast<UItemMetaAsset>(LoadedObject);

		if (MetaAsset)
		{
			LoadedMetaCache.Add(ItemId, MetaAsset);
			SuccessfullyLoadedCount++;
		}
		else
		{
			UE_LOG(LogViewModel, Warning, TEXT("UVM_Inventory::OnMetaDataLoaded - Failed to load/cast metadata for ID: %s. Object: %s. Caching nullptr."), *ItemId.ToString(), *GetNameSafe(LoadedObject));
			LoadedMetaCache.Add(ItemId, nullptr);
		}
	}
	
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::OnMetaDataLoaded - Successfully loaded %d out of %d assets from the completed batch."), SuccessfullyLoadedCount, JustCompletedLoadIds.Num());
	
	if (GetWorld() && DebounceTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(DebounceTimerHandle);
	}
	UE_LOG(LogViewModel, Log, TEXT("Calling PopulateViewModelProperties : %hs"), __FUNCTION__);
	PopulateViewModelProperties();
	
	SetIsLoading(false);
}

void UVM_Inventory::PopulateViewModelProperties()
{
	UE_LOG(LogViewModel, Log, TEXT("PopulateViewModelProperties - Populating lists based on LastKnown data and LoadedMetaCache."));
	
	if (!InventoryComponent.IsValid() || !CustomizationComponent.IsValid())
	{
		UE_LOG(LogViewModel, Warning, TEXT("PopulateViewModelProperties - Components became invalid before population could occur. Clearing VM data."));
		SetInventoryItemsList({});
		SetEquippedItemsMap({});
		return;
	}
	
	HandleEquippedItemsUpdate(CustomizationComponent->GetCurrentCustomizationState());
	TArray<TObjectPtr<UInventoryListItemData>> NewList;
	NewList.Reserve(LastKnownOwnedItems.Num());

	for (const auto& Pair : LastKnownOwnedItems)
	{
		FPrimaryAssetId ItemId = Pair.Key;
		int32 Count = Pair.Value;
		if (!ItemId.IsValid() || Count <= 0) continue;

		if (TObjectPtr<UItemMetaAsset>* FoundMetaPtr = LoadedMetaCache.Find(ItemId))
		{
			if (UItemMetaAsset* MetaAsset = FoundMetaPtr->Get())
			{
				UInventoryListItemData* NewItemObject = NewObject<UInventoryListItemData>(this);

				// ... (SetEventSubscriptionFunctions остается без изменений)

				NewItemObject->InitializeFromMeta(
					MetaAsset,
					Count,
					IsItemSlugEquipped(MetaAsset->GetFName())
				);
				NewList.Add(NewItemObject);
			}
		}
		else
		{
			UE_LOG(LogViewModel, Warning, TEXT("PopulateViewModelProperties: MetaAsset for owned item %s not found in cache. Skipping item in list."), *ItemId.ToString());
		}
	}

	SetInventoryItemsList(NewList);
	SetItemsCount(InventoryItemsList.Num());
}

void UVM_Inventory::BroadcastGetterForType(EItemSlot ItemSlot)
{
	// TODO: maybe map EItemType -> Function Pointer/Name? 
	switch (ItemSlot)
	{
	case EItemSlot::Hat:	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHatItem);
		break;
	case EItemSlot::Body:	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetBodyItem);
		break;
	case EItemSlot::Legs:	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetLegsItem);
		break;
	case EItemSlot::Feet:	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFeetItem);
		break;
	case EItemSlot::Wrists: UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWristsItem);
		break;
		
	default:
		UE_LOG(LogViewModel, Warning, TEXT("BroadcastGetterForType: No specific broadcast handler for EItemSlot %s"), *UEnum::GetValueAsString(ItemSlot));
		break;
	}
}

void UVM_Inventory::SetSkinsForColorPalette(const TArray<USkinListItemData*>& NewSkins)
{
	UE_MVVM_SET_PROPERTY_VALUE(SkinsForColorPalette, NewSkins);
}

void UVM_Inventory::OnSkinMetaDataForPaletteLoaded(TArray<TObjectPtr<USkinListItemData>> LoadedSkinListData)
{
	// Check if this callback is still relevant for the currently selected palette item
	// This check is important if multiple RequestColorPaletteForItem calls happen quickly.
	// The FetchAndPopulateSkinsForPalette callback already does a relevance check, but an extra one here is safe.
	if (ItemSlugForColorPalette == NAME_None && !LoadedSkinListData.IsEmpty())
	{
		UE_LOG(LogViewModel, Log, TEXT("OnSkinMetaDataForPaletteLoaded: Palette was cleared while skins were loading. Discarding %d loaded skins."), LoadedSkinListData.Num());
		SetIsColorPaletteLoading(false); // Ensure loading state is reset
		return;
	}
	// If ItemSlugForColorPalette is set, but the loaded data is for a *different* slug (due to rapid changes),
	// the check inside the CreateLambda of FetchAndPopulateSkinsForPalette should prevent this from being called
	// or prevent it from updating the wrong data.

	UE_LOG(LogViewModel, Log, TEXT("OnSkinMetaDataForPaletteLoaded: Received %d USkinListItemData entries for item '%s'."), LoadedSkinListData.Num(), *ItemSlugForColorPalette.ToString());
	SetSkinsForColorPalette(LoadedSkinListData);
	SetIsColorPaletteLoading(false);
}

void UVM_Inventory::SetIsColorPaletteLoading(const bool InLoadingState)
{
	if (IsColorPaletteLoading != InLoadingState)
	{
		UE_MVVM_SET_PROPERTY_VALUE(IsColorPaletteLoading, InLoadingState);
	}
}

bool UVM_Inventory::GetIsColorPaletteLoading() const
{
	return IsColorPaletteLoading;
}

void UVM_Inventory::RequestColorPaletteForItem(FName MainItemSlug)
{
	UE_LOG(LogViewModel, Log, TEXT("RequestColorPaletteForItem: Requested for item slug '%s'"), *MainItemSlug.ToString());
    if (ItemSlugForColorPalette == MainItemSlug && MainItemSlug != NAME_None)
    {
        if (SkinsForColorPalette.Num() > 0 && !IsColorPaletteLoading) {
             UE_LOG(LogViewModel, Log, TEXT("RequestColorPaletteForItem: Palette already populated for '%s'."), *MainItemSlug.ToString());
            return;
        }
    }
    
    if (MainItemSlug == NAME_None)
    {
        ClearColorPalette();
        return;
    }

    SetItemSlugForColorPalette(MainItemSlug);
    SetSkinsForColorPalette({});      
    SetIsColorPaletteLoading(true);

	
	//FString AssetTypeString = UItemShaderMetaAsset::StaticClass()->GetPrimaryAssetId();
	FPrimaryAssetId MainItemAssetId = FPrimaryAssetId(GLOBAL_CONSTANTS::PrimaryItemAssetType, MainItemSlug);
	TObjectPtr<UItemMetaAsset> FoundAsset = LoadedMetaCache.FindRef(MainItemAssetId);

	if (FoundAsset && FoundAsset->bSupportsSkins && FoundAsset->AvailableSkinAssetIds.Num() > 0)
	{
		const TArray<FPrimaryAssetId>& SkinIds = FoundAsset->AvailableSkinAssetIds;

		TSet<FPrimaryAssetId> SkinIdSet;
		for (const FPrimaryAssetId& SkinId : SkinIds)
		{
			if (SkinId.IsValid())
			{
				SkinIdSet.Add(SkinId);
			}
		}

		if (SkinIdSet.IsEmpty())
		{
			UE_LOG(LogViewModel, Warning, TEXT("RequestColorPaletteForItem: All AvailableSkinAssetIds are invalid for item '%s'."), *MainItemSlug.ToString());
			SetSkinsForColorPalette({});
			SetIsColorPaletteLoading(false);
		}
		else
		{
			FName PaletteSlug = MainItemSlug;

			FOnMetaDataRequestCompleted OnSkinsReady = FOnMetaDataRequestCompleted::CreateLambda(
				[this, PaletteSlug, SkinIds]()
			{
				if (ItemSlugForColorPalette != PaletteSlug)
				{
					UE_LOG(LogViewModel, Log, TEXT("RequestColorPaletteForItem (SkinsLoadCallback): Palette item changed during async load. Ignoring result."));
					return;
				}

				TArray<TObjectPtr<USkinListItemData>> SkinItems;

				for (const FPrimaryAssetId& SkinId : SkinIds)
				{
					if (!SkinId.IsValid()) continue;

					UItemMetaAsset* Meta = LoadedMetaCache.FindRef(SkinId);
					UItemShaderMetaAsset* ShaderMeta = Cast<UItemShaderMetaAsset>(Meta);

					if (ShaderMeta)
					{
						USkinListItemData* SkinData = NewObject<USkinListItemData>(this);
						const bool bEquipped = IsItemSlugEquipped(ShaderMeta->GetFName());
						const bool bIsOwned = InventoryComponent.IsValid() && InventoryComponent->GetOwnedItems().Contains(SkinId);
						
						SkinData->InitializeFromMeta(ShaderMeta, 1, bEquipped);
						SkinData->bIsOwned = bIsOwned;
						
						SkinItems.Add(SkinData);
					}
					else
					{
						UE_LOG(LogViewModel, Warning, TEXT("RequestColorPaletteForItem (SkinsLoadCallback): Failed to cast or find skin for ID: %s"), *SkinId.ToString());
					}
				}

				OnSkinMetaDataForPaletteLoaded(SkinItems);
			});

			RequestMetaDataAndExecute(SkinIdSet, OnSkinsReady);
		}
	}
	else
	{
		// If asset doesn't support skins or has no skin IDs, clear the palette and loading state.
		SetSkinsForColorPalette({});
		SetIsColorPaletteLoading(false);
	}

}

void UVM_Inventory::FetchAndPopulateSkinsForPalette(UItemShaderMetaAsset* MainItemMeta)
{
	if (!MainItemMeta)
    {
        UE_LOG(LogViewModel, Error, TEXT("FetchAndPopulateSkinsForPalette: MainItemMeta is null."));
        if (ItemSlugForColorPalette == (MainItemMeta ? MainItemMeta->GetFName() : NAME_None))
        {
            SetSkinsForColorPalette({});
            SetIsColorPaletteLoading(false);
        }
        return;
    }

    // Ensure this fetch is still for the currently selected item for the palette
    if (ItemSlugForColorPalette != MainItemMeta->GetFName())
    {
        UE_LOG(LogViewModel, Log, TEXT("FetchAndPopulateSkinsForPalette: Palette item changed from '%s' to '%s' while fetching skins. Aborting."),
            *MainItemMeta->GetFName().ToString(), *ItemSlugForColorPalette.ToString());
        // Do not change loading state here, as another request might be in progress for the new ItemSlugForColorPalette
        return;
    }

    UE_LOG(LogViewModel, Log, TEXT("FetchAndPopulateSkinsForPalette: Fetching skins for item '%s'. It has %d available skin asset IDs."),
        *MainItemMeta->GetFName().ToString(), MainItemMeta->AvailableSkinAssetIds.Num());

    if (MainItemMeta->AvailableSkinAssetIds.IsEmpty())
    {
        SetSkinsForColorPalette({});
        SetIsColorPaletteLoading(false);
        return;
    }

    TSet<FPrimaryAssetId> SkinMetaIdsToEnsure;
    for (const FPrimaryAssetId& SkinAssetId : MainItemMeta->AvailableSkinAssetIds)
    {
        if (SkinAssetId.IsValid())
        {
            SkinMetaIdsToEnsure.Add(SkinAssetId);
        }
    }

    if (SkinMetaIdsToEnsure.IsEmpty())
    {
        UE_LOG(LogViewModel, Log, TEXT("FetchAndPopulateSkinsForPalette: No valid skin asset IDs to ensure for '%s'."), *MainItemMeta->GetFName().ToString());
        SetSkinsForColorPalette({});
        SetIsColorPaletteLoading(false);
        return;
    }
    
    // Store the main item's slug at the time of request for validation in the callback
    FName MainItemSlugAtRequest = MainItemMeta->GetFName();

    FOnMetaDataRequestCompleted OnSkinsMetaReadyDelegate = FOnMetaDataRequestCompleted::CreateLambda(
        [this, MainItemMetaRef = TWeakObjectPtr<UItemShaderMetaAsset>(MainItemMeta), MainItemSlugAtRequest]()
    {
        if (!MainItemMetaRef.IsValid())
        {
            UE_LOG(LogViewModel, Warning, TEXT("FetchAndPopulateSkinsForPalette (Callback): MainItemMeta became invalid."));
            if (ItemSlugForColorPalette == MainItemSlugAtRequest) ClearColorPalette();
            return;
        }
        
        // Check if the request is still relevant for the palette
        if (ItemSlugForColorPalette != MainItemSlugAtRequest)
        {
            UE_LOG(LogViewModel, Log, TEXT("FetchAndPopulateSkinsForPalette (Callback): Palette item changed while loading skins for '%s'. Aborting."), *MainItemSlugAtRequest.ToString());
            return;
        }

        TArray<TObjectPtr<USkinListItemData>> SkinListDataObjects;
        for (const FPrimaryAssetId& SkinAssetId : MainItemMetaRef->AvailableSkinAssetIds)
        {
            if (!SkinAssetId.IsValid()) continue;

            UItemMetaAsset* SkinMeta = LoadedMetaCache.FindRef(SkinAssetId);
            if (SkinMeta)
            {
                USkinListItemData* SkinListItem = NewObject<USkinListItemData>(this);
                
                bool bIsThisSkinApplied = false;
                if (CustomizationComponent.IsValid() && ItemSlugForColorPalette != NAME_None)
                {
                    // This logic assumes that "equipping" a skin means its FName (ItemSlug)
                    // will be present in one of the CustomizationComponent's equipped lists.
                    bIsThisSkinApplied = IsItemSlugEquipped(SkinMeta->GetFName());
                }
                
                SkinListItem->InitializeFromMeta(SkinMeta, 1, bIsThisSkinApplied);
                SkinListDataObjects.Add(SkinListItem);
            }
            else
            {
                UE_LOG(LogViewModel, Warning, TEXT("FetchAndPopulateSkinsForPalette (Callback): Meta for skin ID %s not found in cache for main item '%s'."), *SkinAssetId.ToString(), *MainItemSlugAtRequest.ToString());
            }
        }
        OnSkinMetaDataForPaletteLoaded(SkinListDataObjects);
    });
    
    RequestMetaDataAndExecute(SkinMetaIdsToEnsure, OnSkinsMetaReadyDelegate);
}

void UVM_Inventory::ClearColorPalette()
{
	UE_LOG(LogViewModel, Log, TEXT("ClearColorPalette: Clearing color palette selection."));

	SetItemSlugForColorPalette(NAME_None);
	SetSkinsForColorPalette({});
	SetIsColorPaletteLoading(false); 
}



bool UVM_Inventory::ApplySkinToCurrentItem(FName SkinSlugToApply)
{
	
    if (ItemSlugForColorPalette == NAME_None)
    {
        UE_LOG(LogViewModel, Warning, TEXT("ApplySkinToCurrentItem: No item selected for color palette (ItemSlugForColorPalette is None). Cannot apply skin %s."), *SkinSlugToApply.ToString());
        return false;
    }
    if (SkinSlugToApply == NAME_None)
    {
        UE_LOG(LogViewModel, Warning, TEXT("ApplySkinToCurrentItem: SkinSlugToApply is None. Cannot apply to item %s."), *ItemSlugForColorPalette.ToString());
        return false;
    }

    if (!CustomizationComponent.IsValid())
    {
        UE_LOG(LogViewModel, Error, TEXT("ApplySkinToCurrentItem: CustomizationComponent is invalid. Cannot apply skin."));
        return false;
    }

    UE_LOG(LogViewModel, Log, TEXT("ApplySkinToCurrentItem: Requesting to apply skin '%s' to item '%s'."), *SkinSlugToApply.ToString(), *ItemSlugForColorPalette.ToString());
    
    // The core assumption: "Equipping" the skin asset will correctly modify the appearance of the main item.
    // This might involve the CustomizationComponent's logic:
    // 1. The skin asset (identified by SkinSlugToApply) is "equipped".
    // 2. The CustomizationComponent checks its variants. If the skin asset has a variant that requires
    //    the main item (ItemSlugForColorPalette) to be equipped, that variant (e.g., a material override) is applied.
    // OR
    // 3. The skin asset itself is a material, and equipping it targets the correct BodyPartType
    //    of the main item.
    CustomizationComponent->EquipItem(SkinSlugToApply);

    // After applying, update the 'IsEquipped' state for the skin items in the current palette
    // to reflect the change immediately in the UI.
    bool bFoundAppliedSkinInPalette = false;
    for (TObjectPtr<USkinListItemData> SkinData : SkinsForColorPalette)
    {
        if (SkinData)
        {
            bool bIsNowApplied = (SkinData->ItemSlug == SkinSlugToApply);
            if (SkinData->GetIsEquipped() != bIsNowApplied)
            {
                SkinData->SetIsEquipped(bIsNowApplied); 
            }
            if (bIsNowApplied) bFoundAppliedSkinInPalette = true;
        }
    }
    if (!bFoundAppliedSkinInPalette && SkinsForColorPalette.Num() > 0) {
        UE_LOG(LogViewModel, Warning, TEXT("ApplySkinToCurrentItem: Applied skin '%s' was not found among the currently displayed palette items for '%s'."), *SkinSlugToApply.ToString(), *ItemSlugForColorPalette.ToString());
    }

    return true;
}
void UVM_Inventory::SetItemSlugForColorPalette(FName NewSlug)
{
	if (ItemSlugForColorPalette != NewSlug)
	{
		UE_MVVM_SET_PROPERTY_VALUE(ItemSlugForColorPalette, NewSlug);
	}
}


void UVM_Inventory::BeginDestroy()
{
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::BeginDestroy - Cleaning up."));
	UnbindDelegates(); 
	CancelAllMetaRequests(); 

	LoadedMetaCache.Empty();
	LastKnownOwnedItems.Empty();
	LastKnownCustomizationState.ClearAttachedActors();
	LastKnownCustomizationState = FCustomizationContextData();

	Super::BeginDestroy();
}

bool UVM_Inventory::RequestRemoveItem(FPrimaryAssetId ItemId, int32 Count)
{
	//TODO::
	return false;
}

void UVM_Inventory::FilterBySlot(const EItemSlot DesiredSlot)
{
    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("UVM_Inventory::FilterBySlot - THIS (ViewModel) IS INVALID!"));
        return;
    }

    LastFilterType = DesiredSlot; 
	
    if (OnFilterMethodChanged.IsBound())
    {
        UE_LOG(LogTemp, Log, TEXT("UVM_Inventory::FilterBySlot - Broadcasting OnFilterMethodChanged with type: %s"), *UEnum::GetValueAsString(DesiredSlot));
        try
        {
            OnFilterMethodChanged.Broadcast(DesiredSlot);
            UE_LOG(LogTemp, Display, TEXT("UVM_Inventory::FilterBySlot - OnFilterMethodChanged: Broadcast finished successfully."));
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogTemp, Error, TEXT("UVM_Inventory::FilterBySlot - EXCEPTION during OnFilterMethodChanged.Broadcast (std::exception: %s). FilterType: %s"), ANSI_TO_TCHAR(e.what()), *UEnum::GetValueAsString(DesiredSlot));
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("UVM_Inventory::FilterBySlot - UNKNOWN EXCEPTION during OnFilterMethodChanged.Broadcast. FilterType: %s"), *UEnum::GetValueAsString(DesiredSlot));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UVM_Inventory::FilterBySlot - Skipped OnFilterMethodChanged.Broadcast because IsBound was false. FilterType: %s"), *UEnum::GetValueAsString(DesiredSlot));
    }
	
}

void UVM_Inventory::UnbindDelegates()
{
	UE_LOG(LogViewModel, Log, TEXT("UVM_Inventory::UnbindDelegates - Attempting to unbind from components."));
	
	if (CustomizationComponent.IsValid())
	{
		CustomizationComponent->OnEquippedItemsChanged.RemoveAll(this);
	}
	
	if (InventoryComponent.IsValid())
	{
		InventoryComponent->OnInventoryChanged.RemoveAll(this); 
	}
	
	if (GetWorld() && DebounceTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(DebounceTimerHandle);
		DebounceTimerHandle.Invalidate();
		UE_LOG(LogViewModel, Verbose, TEXT("... Cleared DebounceTimerHandle."));
	}
}

void UVM_Inventory::TriggerPopulateViewModelProperties()
{
	GetWorld()->GetTimerManager().SetTimer(
		DebounceTimerHandle,
		this,
		&ThisClass::PopulateViewModelProperties,
		0.3f,
		false
	);
}

void UVM_Inventory::BroadcastEquippedItemChanges()
{
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHatItem);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetBodyItem);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetLegsItem);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFeetItem);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWristsItem);
}

FInventoryEquippedItemData UVM_Inventory::GetHatItem() const
{
	const FInventoryEquippedItemData* FoundItem = EquippedItemsMap.Find(EItemSlot::Hat);
	return FoundItem ? *FoundItem : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UVM_Inventory::GetBodyItem() const
{
	const FInventoryEquippedItemData* FoundItem = EquippedItemsMap.Find(EItemSlot::Body);
	return FoundItem ? *FoundItem : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UVM_Inventory::GetLegsItem() const
{
	const FInventoryEquippedItemData* FoundItem = EquippedItemsMap.Find(EItemSlot::Legs);
	return FoundItem ? *FoundItem : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UVM_Inventory::GetFeetItem() const
{
	const FInventoryEquippedItemData* FoundItem = EquippedItemsMap.Find(EItemSlot::Feet);
	return FoundItem ? *FoundItem : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UVM_Inventory::GetWristsItem() const
{
	const FInventoryEquippedItemData* FoundItem = EquippedItemsMap.Find(EItemSlot::Wrists);
	return FoundItem ? *FoundItem : FInventoryEquippedItemData();
}

void UVM_Inventory::SetIsLoading(bool bNewLoadingState)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsLoading, bNewLoadingState);
}
