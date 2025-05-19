// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerControllerBase.generated.h"

class UCustomizationComponent;
class UInventoryComponent;
class UVM_Inventory;
class UGameInstanceBase;

UCLASS()
class ASYNCCUSTOMISATION_API APlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UVM_Inventory* GetInventoryViewModel() const;
	
protected:
	
	UFUNCTION()
	void OnInventoryViewModelUpdated(UVM_Inventory* InInventoryViewModel,
									 UInventoryComponent* InInventoryComponent,
									 UCustomizationComponent* InCustomizationComponent);

	UPROPERTY()
	UGameInstanceBase* GameInstance;

private:
	UPROPERTY()
	UVM_Inventory* InventoryViewModel;
};
