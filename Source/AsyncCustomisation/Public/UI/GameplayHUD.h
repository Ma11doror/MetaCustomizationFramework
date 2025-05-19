// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayHUD.generated.h"

class UInventoryWidget;
/**
 * 
 */
UCLASS()
class ASYNCCUSTOMISATION_API AGameplayHUD : public AHUD
{
	GENERATED_BODY()

	
public:
	virtual void BeginPlay() override;
	void ShowInventory();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY()
	UUserWidget* InventoryWidgetInstance;
};
