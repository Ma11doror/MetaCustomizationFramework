#include "UI/Inventory/Components/ItemSlotWidget.h"
#include "Components/Core/Data.h"
#include "UI/VM_Inventory.h"


UItemSlotWidget::UItemSlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UItemSlotWidget::SetSlotData(const FInventoryEquippedItemData& InData)
{
    if (InData == FInventoryEquippedItemData{})
    {
        SetDefaults();
        return;
    }

    SetItemNameText(InData.ItemName);
    
    SetItemIcon(InData.Icon.LoadSynchronous());
    SetItemTier(InData.Tier);
}

void UItemSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetDefaults();
}

void UItemSlotWidget::SetDefaults()
{
    SetSlotNameText(DefaultSlotName);
    SetItemNameText(DefaultItemName);
    
    SetItemIcon(DefaultSlotIcon);
    SetItemTier(EItemTier::None);
}
