#include "UI/Inventory/Components/InventoryItemEntryWidget.h"

#include "UI/Inventory/Data/InventoryListItemData.h"


void UInventoryItemEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	
	UE_LOG(LogTemp, Display, TEXT("InventoryItemEntryWidget::NativeOnListItemObjectSet:: %s"), *ListItemObject->GetName());
	
	UInventoryListItemData* ItemData = Cast<UInventoryListItemData>(ListItemObject);
	if (!ItemData)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	if(FilterDelegateHandle.IsValid())
	{
		ItemData->UnsubscribeFromFilterChanges(FilterDelegateHandle);
		FilterDelegateHandle.Reset();
	}
	FilterDelegateHandle = ItemData->SubscribeToFilterChanges(this, 
		[this](EItemType NewFilterType) 
		{
			HandleFilterChanged(NewFilterType);
		}
	);
	
	ItemData->OnIsEquippedChanged.BindUObject(this, &ThisClass::SetIsEquippedVisualState);
}

void UInventoryItemEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
}

void UInventoryItemEntryWidget::NativeDestruct()
{
	
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
