// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PaletteRequester.generated.h"


DECLARE_DELEGATE_OneParam(FOnRequestColorPalette, FName/*, ItemSlug*/);
/**
 * 
 */
UINTERFACE(MinimalAPI)
class UPaletteRequester : public UInterface
{
	GENERATED_BODY()
};


class ASYNCCUSTOMISATION_API IPaletteRequester
{
	GENERATED_BODY()
	
public:
	virtual void SetPaletteRequestHandler(const FOnRequestColorPalette& InDelegate)
	{
	}
};
