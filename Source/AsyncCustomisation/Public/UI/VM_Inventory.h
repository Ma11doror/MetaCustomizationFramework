// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Components/Core/CustomizationTypes.h"
#include "VM_Inventory.generated.h"

class UInventoryListItemData;
struct FInventoryEquippedItemData;
struct FStreamableHandle;
class UInventoryComponent;
class UCustomizationComponent;
class UItemMetaAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogViewModel, Log, All);

USTRUCT(BlueprintType)
struct FInventoryEquippedItemData 
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Equipped Item")
    FName ItemSlug;

    UPROPERTY(BlueprintReadOnly, Category="Equipped Item")
    FName ItemName;

    UPROPERTY(BlueprintReadOnly, Category="Equipped Item")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category="Equipped Item")
    EItemTier Tier = EItemTier::Common;

    bool operator==(const FInventoryEquippedItemData& Other) const
    {
        return ItemSlug.IsEqual(Other.ItemSlug); // &&
            //ItemName.IsEqual(Other.ItemName) &&
            //Tier == Other.Tier;
        // Specify how to compare items here. Maybe items can upgrade tier?
    }
};


DECLARE_DELEGATE(FOnMetaDataRequestCompleted);

struct FPendingMetaRequest
{
    TSet<FPrimaryAssetId> RequestedIds;
    FOnMetaDataRequestCompleted CompletionDelegate;
    TSharedPtr<FStreamableHandle> LoadHandle;

    FPendingMetaRequest(const TSet<FPrimaryAssetId>& InIds, FOnMetaDataRequestCompleted InDelegate)
        : RequestedIds(InIds), CompletionDelegate(InDelegate)
    {
    }
};

inline bool operator==(const TMap<EItemType, FInventoryEquippedItemData>& Lhs, const TMap<EItemType, FInventoryEquippedItemData>& Rhs)
{
    if (Lhs.Num() != Rhs.Num()) return false;
    for (const auto& Pair : Lhs)
    {
        const FInventoryEquippedItemData* BValue = Rhs.Find(Pair.Key);
        if (!BValue || !(Pair.Value == *BValue)) return false;
    }
    return true;
}

inline bool operator==(const TMap<FPrimaryAssetId, int32>& LHS, const TMap<FPrimaryAssetId, int32>& RHS)
{
    if (LHS.Num() != RHS.Num())
    {
        return false;
    }
    for (const auto& Pair : LHS)
    {
        const auto OtherValue = RHS.Find(Pair.Key);
        if (!OtherValue || *OtherValue != Pair.Value)
        {
            return false;
        }
    }
    
    return true;
}

UCLASS()
class ASYNCCUSTOMISATION_API UVM_Inventory : public UMVVMViewModelBase
{
	GENERATED_BODY()


public:
    UVM_Inventory();
    
    friend class APlayerControllerBase;
    friend class UInventoryWidget;
    
    UFUNCTION()
    void SetbIsLoading(const bool bIsLoaded);
    
    UFUNCTION(BlueprintPure, FieldNotify)
    bool IsLoaded() const;
    
    UFUNCTION(BlueprintPure, FieldNotify, Category = "Inventory|UI Helper")
    TArray<UObject*> GetInventoryItemsAsObjects() const;
    
    void FilterBySlot(EItemType DesiredSlotType);
    
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnFilterMethodChanged, EItemType /* InItemType */ );
    FOnFilterMethodChanged OnFilterMethodChanged;

    EItemType GetFilterType() const;
    
protected:
    UFUNCTION(BlueprintCallable, Category = "ViewModel")
    void Initialize(UInventoryComponent* InInventoryComp, UCustomizationComponent* InCustomizationComp);

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|Inventory")
    TArray<TObjectPtr<UInventoryListItemData>> InventoryItemsList;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|Inventory")
    TMap<EItemType, FInventoryEquippedItemData> EquippedItemsMap;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|State")
    bool bIsLoading = false;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    bool RequestEquipItem(FName ItemSlugToEquip);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    bool RequestUnequipItem(FName ItemSlugToUnequip);
    
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    void OnEntryItemClicked(FName ItemSlugToUnequip);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    void RequestUnequipSlot(ECustomizationSlotType SlotToUnequip);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    void RequestDropItem(FPrimaryAssetId ItemIdToDrop, int32 CountToDrop = 1);

    void SetInventoryItemsList(const TArray<TObjectPtr<UInventoryListItemData>>& InNewList);
    void SetEquippedItemsMap(const TMap<EItemType, FInventoryEquippedItemData>& NewMap);
    void SetItemsCount(const int32 NewCount);
    void SetIsLoading(bool bNewLoadingState);

    UPROPERTY(Transient)
    TWeakObjectPtr<UInventoryComponent> InventoryComponent;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCustomizationComponent> CustomizationComponent;
    
    UFUNCTION()
    void HandleInventoryDeltaUpdate(UItemMetaAsset* InItem, const int32 CountDelta);
    
    void RequestMetaDataAndExecute(const TSet<FPrimaryAssetId>& MetaAssetIdsToEnsure, FOnMetaDataRequestCompleted OnCompleteDelegate);
    void CheckLoadingState();
    void OnMetaDataRequestCompleted(int32 RequestIndex);
    void CancelAllMetaRequests();
    
    bool IsItemSlugEquipped(FName ItemSlug);    

    UFUNCTION()
    void HandleEquippedItemsUpdate(const FCustomizationContextData& NewState);
  
    void RefreshAllViewModelData(); 
    void LoadRequiredMetaData(const TArray<FPrimaryAssetId>& MetaAssetIdsToLoad);
    void OnMetaDataLoaded();
    void PopulateViewModelProperties();
    void BroadcastGetterForType(EItemType ItemType);

  
    UPROPERTY(Transient)
    TMap<FPrimaryAssetId, TObjectPtr<UItemMetaAsset>> LoadedMetaCache;


    TSharedPtr<FStreamableHandle> CurrentMetaLoadHandle;
    TArray<FPrimaryAssetId> PendingMetaLoadIds; 

    UPROPERTY(Transient)
	TMap<FPrimaryAssetId, int32> LastKnownOwnedItems;

    UPROPERTY(Transient)
    FCustomizationContextData LastKnownCustomizationState;

    virtual void BeginDestroy() override;

    bool RequestRemoveItem(FPrimaryAssetId ItemId, int32 Count);

    TArray<FPendingMetaRequest> ActiveMetaRequests;

private:

    void UnbindDelegates();
    
    FTimerHandle DebounceTimerHandle;
    void TriggerPopulateViewModelProperties();
    
    // TODO:: ?
    UPROPERTY(BlueprintReadWrite, EditAnywhere, FieldNotify, Setter, Category = "ViewModel|Inventory", meta = (AllowPrivateAccess))
    int32 ItemsCount;

    EItemType LastFilterType = EItemType::None;
    
    void BroadcastEquippedItemChanges();


    /*
     *  Hardcoded each slot for now. TODO:: think about template or marco for generate functions for each slot
     */
    UFUNCTION(BlueprintPure, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData GetHatItem() const;

    UFUNCTION(BlueprintPure, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData GetBodyItem() const;

    UFUNCTION(BlueprintPure, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData GetLegsItem() const;
    
    UFUNCTION(BlueprintPure, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData GetFeetItem() const;
    
    UFUNCTION(BlueprintPure, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData GetWristsItem() const;
        
    UFUNCTION(BlueprintPure, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData GetEquippedSkinItem() const;

    UPROPERTY(BlueprintGetter=GetFeetItem, FieldNotify, Category="MVVM|Inventory")
    FInventoryEquippedItemData FeetItem;
    
};
