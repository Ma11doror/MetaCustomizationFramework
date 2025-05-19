// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInstanceBase.generated.h"

class UMVVMViewModelBase;
class UCustomizationComponent;
class UInventoryComponent;
class UVM_Inventory;
/**
 * 
 */
UCLASS()
class ASYNCCUSTOMISATION_API UGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryViewModelUpdated, UVM_Inventory*, InventoryViewModel,
											   UInventoryComponent*, InventoryComponent,
											   UCustomizationComponent*, CustomizationComponent);
	
public:
	static UGameInstanceBase* Get(const UObject* InWorldContextObject, const bool IsRequired);
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryViewModelUpdated OnInventoryViewModelUpdated;

	UFUNCTION(BlueprintCallable)
	void RequestViewModel(APlayerController* InController);
	
protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetupViewModel(UMVVMViewModelBase* InModel);

	
	TObjectPtr<UVM_Inventory> InventoryViewModel = nullptr;


};
