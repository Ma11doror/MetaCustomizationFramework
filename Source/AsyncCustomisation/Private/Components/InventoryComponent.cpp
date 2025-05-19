#include "Components/InventoryComponent.h"

#include "Components/Core/CustomizationSlotTypes.h"
#include "Components/CustomizationComponent.h"
#include "Utilities/CustomizationAssetManager.h"
#include "BaseCharacter.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    if (!OwningCharacter.IsValid() || !OwningCharacter->CustomizationComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("%hs failed to get owner"), __FUNCTION__);
        return;
    }
}

void UInventoryComponent::EquipItem(const FName& ItemSlug)
{
    if (ItemSlug.IsNone())
    {
        return;
    }
    
    OwningCharacter->CustomizationComponent->EquipItem(ItemSlug);
}

void UInventoryComponent::UnequipItem(const FName& ItemSlug)
{
    if (ItemSlug.IsNone())
    {
        return;
    }
    
    if (!OwningCharacter.IsValid() || !OwningCharacter->CustomizationComponent)
    {
        return;
    }
    
    OwningCharacter->CustomizationComponent->UnequipItem(ItemSlug);
}

void UInventoryComponent::AddItem(UItemMetaAsset* Item, const int32 Count)
{
    if (!Item || Count == 0)
    {
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("%hs AddItem: %s | count: %d"), __FUNCTION__, *Item->Name.ToString(), Count);
    
    OwnedItems.Add(Item->GetPrimaryAssetId(), Count);
    OnInventoryChanged.Broadcast(Item, Count);
}

void UInventoryComponent::DropItem(UItemMetaAsset* Item, const int32 Count)
{
    UE_LOG(LogTemp, Warning, TEXT("%hs Drop item function is not implemented! Item: %s | count: %d"), __FUNCTION__, *Item->Name.ToString(), Count);

    //TODO:: 
}
