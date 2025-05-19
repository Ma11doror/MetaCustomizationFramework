#pragma once

#include "CoreMinimal.h"
#include "Components/Core/CharacterComponentBase.h"
#include "InventoryComponent.generated.h"


class UItemMetaAsset;



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ASYNCCUSTOMISATION_API UInventoryComponent : public UCharacterComponentBase
{
    GENERATED_BODY()

public:
    UInventoryComponent();
    
    virtual void BeginPlay() override;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, UItemMetaAsset*, MetaAsset, const int32, Count);
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryChanged OnInventoryChanged;
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void EquipItem(const FName& ItemSlug);
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UnequipItem(const FName& ItemSlug);
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(UItemMetaAsset *Item, const int32 Count);

    auto GetOwnedItems() { return OwnedItems; }

protected:

    UPROPERTY()
    TMap<FPrimaryAssetId, int32> OwnedItems;
    
}; 