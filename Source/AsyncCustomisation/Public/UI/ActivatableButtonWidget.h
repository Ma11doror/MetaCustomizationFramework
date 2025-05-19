// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ActivatableButtonWidget.generated.h"

UCLASS()
class ASYNCCUSTOMISATION_API UActivatableButtonWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimpleButtonBaseGroupDelegate, UUserWidget*, AssociatedButton);
	
public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FSimpleButtonBaseGroupDelegate OnButtonBaseClicked;
};
