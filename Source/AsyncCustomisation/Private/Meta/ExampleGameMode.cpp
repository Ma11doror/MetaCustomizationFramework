// Fill out your copyright notice in the Description page of Project Settings.


#include "Meta/ExampleGameMode.h"

#include "BaseCharacter.h"
#include "Components/CustomizationComponent.h"


void AExampleGameMode::EquipSomething(ABaseCharacter* Player, const TArray<FName>& Items)
{
	if(!Player) { return; }
	if(Items.Num() < 1) { return; }
	
	Cast<ABaseCharacter>(Player)->CustomizationComponent->EquipItems(Items);
}

void AExampleGameMode::UnequipSomething(ABaseCharacter* Player, const FName& Items)
{
	if(!Player) { return; }
	if(Items.IsNone()) { return; }
	
	Cast<ABaseCharacter>(Player)->CustomizationComponent->UnequipItem(Items);
}
