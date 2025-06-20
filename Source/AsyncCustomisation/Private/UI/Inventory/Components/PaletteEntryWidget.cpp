// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/Components/PaletteEntryWidget.h"

#include "UI/Inventory/Data/InventoryListItemData.h"

void UPaletteEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	ensureAlways(ListItemObject);
	InventoryListItemData = Cast<USkinListItemData>(ListItemObject);
	ensureAlways(InventoryListItemData);
	
	if (InventoryListItemData)
	{
		InventoryListItemData->OnItemDataChanged.AddUObject(this, &UPaletteEntryWidget::HandleItemDataChanged);
	}
	
	UpdateUIData(InventoryListItemData);
}

void UPaletteEntryWidget::NativeOnEntryReleased()
{
	if (InventoryListItemData)
	{
		InventoryListItemData->OnItemDataChanged.RemoveAll(this);
	}
	IUserObjectListEntry::NativeOnEntryReleased();
}

void UPaletteEntryWidget::HandleItemDataChanged()
{
	UpdateUIData(InventoryListItemData);
}
