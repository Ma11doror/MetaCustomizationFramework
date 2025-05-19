#include "UI/Inventory/InventoryWidget.h"
#include "UI/Inventory/Components/ItemSlotWidget.h"
#include "Components/VerticalBox.h"
#include "CommonUI/Public/CommonAnimatedSwitcher.h"
#include "CommonUI/Public/CommonListView.h"
#include "Meta/PlayerControllerBase.h"
#include "UI/VM_Inventory.h"
#include "UI/Inventory/Data/InventoryListItemData.h"


UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UInventoryWidget::NativeDestruct()
{
    Super::NativeDestruct();
 
}

void UInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    PlayerController = GetOwningPlayer<APlayerControllerBase>();
    InventoryViewModel = PlayerController->GetInventoryViewModel();
    ensureAlways(InventoryViewModel);
}

void UInventoryWidget::NativeOnActivated()
{
    Super::NativeOnActivated();

    if (!InventorySlotsButtonGroup)
    {
        ConstructButtons();
    }
    InventorySlotsButtonGroup->OnButtonBaseClicked.AddUniqueDynamic(this, &ThisClass::OnItemSlotClicked);
    ItemsList->OnItemClicked().AddUObject(this, &ThisClass::OnListItemClicked);

    BackButton->OnClicked().AddUObject(this, &ThisClass::OnBackButtonClicked);
}

void UInventoryWidget::NativeOnDeactivated()
{
    InventorySlotsButtonGroup->OnButtonBaseClicked.Clear();
    
    Super::NativeOnDeactivated();
}

void UInventoryWidget::ClearFilter()
{
}

void UInventoryWidget::OnItemSlotClicked(UCommonButtonBase* AssociatedButton, int32 ButtonIndex)
{
    auto* SlotButton = Cast<UItemSlotWidget>(AssociatedButton);
    ensure(SlotButton);
    ensureAlwaysMsgf(InventoryViewModel, TEXT("UInventoryWidget::OnItemSlotClicked: InventoryViewModel is null!"));
    ensureAlwaysMsgf(InventorySwitcher, TEXT("UInventoryWidget::OnItemSlotClicked: InventorySwitcher is null!"));

    InventoryViewModel->FilterBySlot(SlotButton->GetSlotType()); 
    InventorySwitcher->SetActiveWidgetIndex(1);
}

void UInventoryWidget::ConstructButtons()
{
    InventorySlotsButtonGroup = NewObject<UCommonButtonGroupBase>(this);
    //InventoryButtonGroup->AddWidgets({PantsButton, HatButton, WristButton, BodyButton, FeetButton});
    TArray<UWidget*> TabList;
    TabList.Reserve( LeftTabsBox->GetAllChildren().Num() + RightTabsBox->GetAllChildren().Num() );
    TabList.Append(LeftTabsBox->GetAllChildren());
    TabList.Append(RightTabsBox->GetAllChildren());
    for (auto Child : TabList)
    {
        if (UCommonButtonBase* Button = Cast<UCommonButtonBase>(Child))
        {
            //Button->SetIsSelectable(false);
           // Button->SetIsToggleable(false);
            InventorySlotsButtonGroup->AddWidget(Child);
        }
    }
    InventorySlotsButtonGroup->SetSelectionRequired(false);
}

void UInventoryWidget::OnListItemClicked(UObject* ItemObject)
{
    ensureAlways(ItemObject);
    
    auto InventoryListItemObject = Cast<UInventoryListItemData>(ItemObject);
    ensureAlways(InventoryListItemObject);
    InventoryViewModel->OnEntryItemClicked(InventoryListItemObject->ItemSlug);
    
    //if (!InventoryViewModel->RequestEquipItem())
    //{
    //    PlayFailEquipAnimation();
    // }
}

void UInventoryWidget::OnBackButtonClicked()
{
    // For correct implementation of BackButton logic we need UI framework which maintains multiple HUD states and manages layers for windows and popups
    // However, I'm not going implement this feature in this project. So for now just a primitive hardcoded solution.


    const EInventorySwitcherTab ActiveTab = static_cast<EInventorySwitcherTab>(InventorySwitcher->GetActiveWidgetIndex());

    if (ActiveTab == EInventorySwitcherTab::SlotsView)
    {
        // Close inventory window
    }
    else
    {
        SetActiveInventoryTab(EInventorySwitcherTab::SlotsView);
    }


    /*switch (static_cast<EInventorySwitcherTab>(InventorySwitcher->GetActiveWidgetIndex()))
    {
    case EInventorySwitcherTab::SlotsView:
        //Close inventory window
        break;
    case EInventorySwitcherTab::ItemsListView:
        InventorySwitcher->SetActiveWidgetIndex(0);
        break;
    case EInventorySwitcherTab::NoItemsView:
        InventorySwitcher->SetActiveWidgetIndex(0);
        break;
    default:
        //Close inventory window? 
        InventorySwitcher->SetActiveWidgetIndex(0);
    }
    */
}

void UInventoryWidget::SetActiveInventoryTab(EInventorySwitcherTab Tab)
{
    if (!InventorySwitcher)
    {
        return;
    }

    switch (Tab)
    {
    case EInventorySwitcherTab::SlotsView:
        InventorySwitcher->SetActiveWidgetIndex(0);
        break;
    case EInventorySwitcherTab::ItemsListView:
        ItemsList->GetDisplayedEntryWidgets().Num() > 0
            ? InventorySwitcher->SetActiveWidgetIndex(1)
            : InventorySwitcher->SetActiveWidgetIndex(2);
        
        break;
    case EInventorySwitcherTab::NoItemsView:
        InventorySwitcher->SetActiveWidgetIndex(2);
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown EInventorySwitcherTab value!"));
    }
}
