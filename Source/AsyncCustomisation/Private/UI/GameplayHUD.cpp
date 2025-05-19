// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GameplayHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/Inventory/InventoryWidget.h"

void AGameplayHUD::BeginPlay()
{
	Super::BeginPlay();
	ShowInventory();
}

void AGameplayHUD::ShowInventory()
{
	if (!InventoryWidgetInstance && InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->AddToViewport();
		}
	}
}
