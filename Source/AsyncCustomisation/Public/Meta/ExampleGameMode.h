// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ExampleGameMode.generated.h"

class ABaseCharacter;

UCLASS()
class ASYNCCUSTOMISATION_API AExampleGameMode : public AGameMode
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void EquipSomething(ABaseCharacter* Player, const TArray<FName>& Items);
	
	UFUNCTION(BlueprintCallable)
	void UnequipSomething(ABaseCharacter* Player, const FName& Items);
};
