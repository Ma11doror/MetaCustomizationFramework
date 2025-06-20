#include "UI/Inventory/InventoryWidget.h"

#include "UI/Inventory/Components/ItemSlotWidget.h"
#include "Components/VerticalBox.h"
#include "CommonUI/Public/CommonAnimatedSwitcher.h"
#include "CommonUI/Public/CommonListView.h"
#include "CommonUI/Public/CommonTileView.h"
#include "Components/Overlay.h"
#include "Meta/PlayerControllerBase.h"
#include "UI/VM_Inventory.h"
#include "UI/Inventory/Components/InventoryItemEntryWidget.h"
#include "UI/Inventory/Data/InventoryListItemData.h"

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
    
    ensureAlways(InventorySlotsButtonGroup);
    InventorySlotsButtonGroup->OnButtonBaseClicked.AddUniqueDynamic(this, &ThisClass::OnItemSlotClicked);
    
    ensureAlways(ItemsList);
    ItemsList->OnItemClicked().AddUObject(this, &ThisClass::OnListItemClicked);
    ItemsList->OnEntryWidgetGenerated().AddUObject(this, &ThisClass::OnMainListItemObjectSet);
    //ItemsList->OnItemIsHoveredChanged().AddUObject(this, &ThisClass::OnItemIsHoveredChanged);
    
    ensureAlways(InventorySlotsButtonGroup);
    OnRequestColorPalette.BindUObject(this, &ThisClass::UInventoryWidget::HandleRequestColorPalette);
    
    ensureAlways(SkinPaletteListView);
    SkinPaletteListView->OnItemClicked().AddUObject(this, &ThisClass::OnPaletteItemClicked);
    
    ensureAlways(BackButton);
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
    
    SetActiveInventoryTab(EInventorySwitcherTab::ItemsListView);
    InventorySwitcher->OnTransitioningChanged.AddWeakLambda(this, [this, SlotButton, InModel = InventoryViewModel](const bool IsTransitioning)
    {
        if (IsTransitioning) return;

        if (!IsValid(InModel) || !IsValid(SlotButton) || !IsValid(this))
        {
            UE_LOG(LogTemp, Warning, TEXT("UInventoryWidget::OnItemSlotClicked - InventorySwitcher transition finished, but some captured objects are invalid."));
        }
            
        InModel->FilterBySlot(SlotButton->GetSlotType());
        OnListTabOpened();
           
        if (IsValid(InventorySwitcher)) 
        {
             InventorySwitcher->OnTransitioningChanged.RemoveAll(this);
        }
    });
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
        
        /*
         * WORKAROUND: Addresses a visual glitch in the WidgetSwitcher where items from the
         * previously active tab remain visible for a single frame after switching to a new
         * items tab.
         * To mitigate this, when navigating away from an items tab (e.g., back to the main
         * inventory view or another category), the filter is cleared. This ensures
         * all items are initially hidden when a new items tab is subsequently opened,
         * preventing the "ghosting" of previous tab's items before the new filter is applied.
         */
        InventoryViewModel->FilterBySlot(EItemSlot::None);
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

void UInventoryWidget::OnListTabOpened()
{
    const auto Found = ItemsList->GetDisplayedEntryWidgets().FindByPredicate([](const UUserWidget* Item)
    {
        return IsValid(Item) && Item->GetVisibility() != ESlateVisibility::Collapsed;
    });
    EmptyListPlaceholder->SetVisibility(Found
                                            ? ESlateVisibility::Collapsed
                                            : ESlateVisibility::Visible);
}

void UInventoryWidget::HandleRequestColorPalette(FName ItemSlug)
{
    if (InventoryViewModel)
    {
        const FName CurrentPaletteSlug = InventoryViewModel->GetItemSlugForColorPalette();
        if (CurrentPaletteSlug != NAME_None && CurrentPaletteSlug == ItemSlug)
        {
            PlayClosePaletteAnimation();
        }
        else
        {
            //InventoryViewModel->RequestColorPaletteForItem(ItemSlug);
            PlayOpenPaletteAnimation(ItemSlug);
        }
    }
}

void UInventoryWidget::OnMainListItemObjectSet(UUserWidget& InEntryWidget)
{
    //UUserWidget* EntryWidget = ItemsList->GetEntryWidgetFromItem(ListItemObject);

    if (IPaletteRequester* ItemEntry = Cast<IPaletteRequester>(&InEntryWidget))
    {
        ItemEntry->SetPaletteRequestHandler(OnRequestColorPalette);
    }
}

void UInventoryWidget::OnPaletteItemClicked(UObject* Item)
{
    ensureAlways(Item);
    
    auto InventoryListItemObject = Cast<USkinListItemData>(Item);
    ensureAlways(InventoryListItemObject);
    InventoryViewModel->OnEntryItemClicked(InventoryListItemObject->ItemSlug);
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
        InventorySwitcher->SetActiveWidgetIndex(1);
        //OnListTabOpened();
        //GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::OnListTabOpened);
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown EInventorySwitcherTab value!"));
    }
}

void UInventoryWidget::FinalizePaletteClosure()
{
    if (InventoryViewModel)
    {
        InventoryViewModel->ClearColorPalette();
    }
}

void UInventoryWidget::FinalizePaletteOpening(const FName& InItemSlug)
{
    if (InventoryViewModel)
    {
        InventoryViewModel->RequestColorPaletteForItem(InItemSlug);
    }
}
