// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Components/Core/CustomizationTypes.h"
#include "VM_Inventory.generated.h"

class USkinListItemData;
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
    
    UPROPERTY(BlueprintReadOnly, Category="Equipped Item")
    FName AppliedSkinSlug;
    
    bool operator==(const FInventoryEquippedItemData& Other) const
    {
        return ItemSlug.IsEqual(Other.ItemSlug) && AppliedSkinSlug.IsEqual(Other.AppliedSkinSlug); // &&
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

inline bool operator==(const TMap<EItemSlot, FInventoryEquippedItemData>& Lhs, const TMap<EItemSlot, FInventoryEquippedItemData>& Rhs)
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

    UFUNCTION(BlueprintPure, FieldNotify, Category = "ViewModel|Inventory")
    TArray<USkinListItemData*> GetSkinsForColorPalette() const;
    
    UFUNCTION()
    void SetbIsLoading(const bool bIsLoaded);
    
    UFUNCTION(BlueprintPure, FieldNotify)
    bool IsLoaded() const;
    
    UFUNCTION(BlueprintPure, FieldNotify, Category = "Inventory|UI Helper")
    TArray<UObject*> GetInventoryItemsAsObjects() const;
    
    void FilterBySlot(EItemSlot DesiredSlot);
    
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnFilterMethodChanged, EItemSlot /* InItemType */ );
    FOnFilterMethodChanged OnFilterMethodChanged;

    EItemSlot GetFilterType() const;

    UFUNCTION(BlueprintPure, Category = "ViewModel|State")
    FName GetItemSlugForColorPalette() const;

protected:
    UFUNCTION(BlueprintCallable, Category = "ViewModel")
    void Initialize(UInventoryComponent* InInventoryComp, UCustomizationComponent* InCustomizationComp);

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|Inventory")
    TArray<TObjectPtr<UInventoryListItemData>> InventoryItemsList;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|Inventory")
    TMap<EItemSlot, FInventoryEquippedItemData> EquippedItemsMap;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|State")
    bool bIsLoading = false;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|State")
    bool IsColorPaletteLoading  = false;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    bool RequestEquipItem(const FName& ItemSlugToEquip);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    bool RequestUnequipItem(const FName& ItemSlugToUnequip);
    
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    void OnEntryItemClicked(const FName& InItemSlug);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    void RequestUnequipSlot(ECustomizationSlotType SlotToUnequip);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Actions")
    void RequestDropItem(FPrimaryAssetId ItemIdToDrop, int32 CountToDrop = 1);

    void SetInventoryItemsList(const TArray<TObjectPtr<UInventoryListItemData>>& InNewList);
    void SetEquippedItemsMap(const TMap<EItemSlot, FInventoryEquippedItemData>& NewMap);
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
    
    bool IsItemSlugEquipped(const FName& ItemSlug);    

    UFUNCTION()
    void HandleEquippedItemsUpdate(const FCustomizationContextData& NewState);
  
    void RefreshAllViewModelData(); 
    void LoadRequiredMetaData(const TArray<FPrimaryAssetId>& MetaAssetIdsToLoad);
    void OnMetaDataLoaded();
    void PopulateViewModelProperties();
    void BroadcastGetterForType(EItemSlot ItemSlot);

  
    UPROPERTY(Transient)
    TMap<FPrimaryAssetId, TObjectPtr<UItemMetaAsset>> LoadedMetaCache;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|Inventory")
    TArray<USkinListItemData*> SkinsForColorPalette;

    void SetSkinsForColorPalette(const TArray<USkinListItemData*>& NewSkins);
    void OnSkinMetaDataForPaletteLoaded(TArray<TObjectPtr<USkinListItemData>> LoadedSkinListData);
    void SetIsColorPaletteLoading(bool InLoadingState);
    bool GetIsColorPaletteLoading() const;
    void RequestColorPaletteForItem(FName MainItemSlug);
    void FetchAndPopulateSkinsForPalette(UItemShaderMetaAsset* MainItemMeta);
    void ClearColorPalette();
    bool ApplySkinToCurrentItem(FName SkinSlugToApply);
    void SetItemSlugForColorPalette(FName NewSlug);

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Category = "ViewModel|State")
    FName ItemSlugForColorPalette;
    
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

    EItemSlot LastFilterType = EItemSlot::None;
    
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
};
