// Fill out your copyright notice in the Description page of Project Settings.


#include "Meta/GameInstanceBase.h"

#include "Components/CustomizationComponent.h"
#include "Components/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/VM_Inventory.h"


void UGameInstanceBase::RequestViewModel(APlayerController* InController)
{
	APawn* Pawn = InController->GetPawn();
	UInventoryComponent* InventoryComponent = Pawn->FindComponentByClass<UInventoryComponent>();
	UCustomizationComponent* CustomizationComponent = Pawn->FindComponentByClass<UCustomizationComponent>();

	if (!InventoryViewModel)
	{
		InventoryViewModel = NewObject<UVM_Inventory>(InController);
		SetupViewModel(InventoryViewModel);
	}

	OnInventoryViewModelUpdated.Broadcast(InventoryViewModel, InventoryComponent, CustomizationComponent);
}

UGameInstanceBase* UGameInstanceBase::Get(const UObject* InWorldContextObject, const bool IsRequired)
{
	ensureAlways(InWorldContextObject);

	auto* RawGameInstance = UGameplayStatics::GetGameInstance(InWorldContextObject);
	auto* GameInstance = Cast<UGameInstanceBase>(RawGameInstance);
	if (IsRequired)
	{
		check(GameInstance);
	}

	return GameInstance;
}
