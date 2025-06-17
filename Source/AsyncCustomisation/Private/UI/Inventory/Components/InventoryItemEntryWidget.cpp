#include "UI/Inventory/Components/InventoryItemEntryWidget.h"

#include "UI/Inventory/Data/InventoryListItemData.h"


void UInventoryItemEntryWidget::SetPaletteRequestHandler(const FOnRequestColorPalette& InDelegate)
{
	IPaletteRequester::SetPaletteRequestHandler(InDelegate);
	PaletteRequestHandler = InDelegate;
}

void UInventoryItemEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	
	UE_LOG(LogTemp, Display, TEXT("InventoryItemEntryWidget::NativeOnListItemObjectSet:: %s"), *ListItemObject->GetName());
	
	InventoryListItemData = Cast<UInventoryListItemData>(ListItemObject);;
	if (!InventoryListItemData)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	if(FilterDelegateHandle.IsValid())
	{
		InventoryListItemData->UnsubscribeFromFilterChanges(FilterDelegateHandle);
		FilterDelegateHandle.Reset();
	}
	FilterDelegateHandle = InventoryListItemData->SubscribeToFilterChanges(this, 
		[this](EItemSlot NewFilterType) 
		{
			HandleFilterChanged(NewFilterType);
		}
	);
	
	UpdateUI(InventoryListItemData);
	
	InventoryListItemData->OnIsEquippedChanged.BindUObject(this, &ThisClass::SetIsEquippedVisualState);
}

void UInventoryItemEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
}

void UInventoryItemEntryWidget::NativeDestruct()
{
	PaletteButton->OnClicked().RemoveAll(this);
	Super::NativeDestruct();
}

void UInventoryItemEntryWidget::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();

	UInventoryListItemData* ItemData = Cast<UInventoryListItemData>(GetListItem());
	if (ItemData && FilterDelegateHandle.IsValid())
	{
		ItemData->UnsubscribeFromFilterChanges(FilterDelegateHandle);
	}
	FilterDelegateHandle.Reset();
	ItemData->OnIsEquippedChanged.Unbind();
}

void UInventoryItemEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (PaletteButton)
	{
		PaletteButton->OnClicked().AddUObject(this, &ThisClass::OnCustomizeButtonClicked);
	}
}

void UInventoryItemEntryWidget::OnCustomizeButtonClicked() const
{
	PaletteRequestHandler.ExecuteIfBound(InventoryListItemData->ItemSlug);
}
