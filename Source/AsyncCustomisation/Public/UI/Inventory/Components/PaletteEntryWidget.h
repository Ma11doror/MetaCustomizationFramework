// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "PaletteEntryWidget.generated.h"

class USkinListItemData;
/**
 * 
 */
UCLASS()
class ASYNCCUSTOMISATION_API UPaletteEntryWidget :  public UCommonButtonBase, public IUserObjectListEntry
{
	GENERATED_BODY()
public:



protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateUIData(USkinListItemData* InData);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USkinListItemData* InventoryListItemData;
};
