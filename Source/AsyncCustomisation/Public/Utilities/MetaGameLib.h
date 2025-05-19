// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/VM_Inventory.h"
#include "MetaGameLib.generated.h"

enum class EDataTableLibraryType : uint8;
enum class ESomatotype : uint8;

UCLASS()
class ASYNCCUSTOMISATION_API UMetaGameLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	
public:
	
	UFUNCTION(BlueprintPure)
	static FPrimaryAssetId GetDefaultSkinAssetIdBySomatotype(const ESomatotype InType);

	UFUNCTION(BlueprintCallable)
	static UDataTable* GetDataTableFromLibrary(EDataTableLibraryType InType);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion")
	static FInventoryEquippedItemData ConvertEquippedMapToSlot(const TMap<EItemType, FInventoryEquippedItemData>& InMap, const EItemType& InType);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion")
	static FInventoryEquippedItemData ConvertEquippedMapHatSlotItem(const TMap<EItemType, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion")
	static FInventoryEquippedItemData ConvertEquippedMapBodySlotItem(const TMap<EItemType, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion")
	static FInventoryEquippedItemData ConvertEquippedMapGlovesSlotItem(const TMap<EItemType, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion")
	static FInventoryEquippedItemData ConvertEquippedPantsPantsSlotItem(const TMap<EItemType, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion")
	static FInventoryEquippedItemData ConvertEquippedMapFeetSlotItem(const TMap<EItemType, FInventoryEquippedItemData>& InMap);
};
