// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Core/Somatotypes.h"
#include "GameFramework/Character.h"
#include "Components/Core/CharacterComponentBase.h"
#include "Components/CustomizationComponent.h"
#include "Components/InventoryComponent.h"
#include "BaseCharacter.generated.h"

enum class ESomatotype : uint8;
class UCustomizationComponent;

UCLASS()
class ASYNCCUSTOMISATION_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UClass* AnimBP;
	
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCustomizationComponent> CustomizationComponent = nullptr;

	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInventoryComponent> InventoryComponent = nullptr;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Character)
	ESomatotype Somatotype = ESomatotype::None;

	UFUNCTION(BlueprintCallable)
	void EquipItems(TArray<FName> InItems);
};
