// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncCustomisation/Public/BaseCharacter.h"

#include "Components/CustomizationComponent.h"
#include "Components/InventoryComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	

	CustomizationComponent = CreateDefaultSubobject<UCustomizationComponent>(TEXT("CustomizationComponent"));
	ensure(CustomizationComponent);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseCharacter::EquipItems(TArray<FName> InItems)
{
	CustomizationComponent->EquipItems(InItems);
}

