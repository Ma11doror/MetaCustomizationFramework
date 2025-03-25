// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomizationItemBase.generated.h"

UCLASS()
class ASYNCCUSTOMISATION_API ACustomizationItemBase : public AActor
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetItemSimulatePhysics(bool bSimulatePhysics);
};
