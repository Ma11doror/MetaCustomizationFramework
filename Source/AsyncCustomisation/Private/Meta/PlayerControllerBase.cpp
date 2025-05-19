// Fill out your copyright notice in the Description page of Project Settings.


#include "Meta/PlayerControllerBase.h"
#include "Meta/GameInstanceBase.h"
#include "UI/VM_Inventory.h"


void APlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	GameInstance = GetGameInstance<UGameInstanceBase>();
	ensureAlways(GameInstance);
	
	GameInstance->OnInventoryViewModelUpdated.AddDynamic(this, &ThisClass::OnInventoryViewModelUpdated);
	GameInstance->RequestViewModel(this);
}

void APlayerControllerBase::BeginDestroy()
{
	if (GameInstance)
	{
		GameInstance->OnInventoryViewModelUpdated.RemoveDynamic(this, &ThisClass::OnInventoryViewModelUpdated);
	}
	
	Super::BeginDestroy();
}

UVM_Inventory* APlayerControllerBase::GetInventoryViewModel() const
{
	ensureAlwaysMsgf(InventoryViewModel, TEXT("APlayerControllerBase::GetInventoryViewModel InventoryViewModel is invalid"));
	return InventoryViewModel;
}

void APlayerControllerBase::OnInventoryViewModelUpdated(UVM_Inventory* InInventoryViewModel, UInventoryComponent* InInventoryComponent, UCustomizationComponent* InCustomizationComponent)
{
	ensureAlways(InInventoryViewModel);
	InventoryViewModel = InInventoryViewModel;
	InventoryViewModel->Initialize(InInventoryComponent, InCustomizationComponent);
}
