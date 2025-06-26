// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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

	// TODO:: for now I had to write converters for each slot for correct get data from map to exact slot. UMG Viewmodel can not be bound with exact parameter if we bind 1 slot to map with keys.
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="HatMapConverter", CompactNodeTitle="HatMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapHat(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="TorsoMapConverter", CompactNodeTitle="TorsoMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapTorso(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="WristsMapConverter", CompactNodeTitle="WristsMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapWrists(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="LegsMapConverter", CompactNodeTitle="LegsMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapLegs(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="FeetMapConverter", CompactNodeTitle="FeetMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapFeet(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="AccessoryMapConverter", CompactNodeTitle="AccessoryMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapAccessory(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
    
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="BackMapConverter", CompactNodeTitle="BackMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapBack(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="HeadMapConverter", CompactNodeTitle="HeadMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapHead(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="SkinMapConverter", CompactNodeTitle="SkinMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapSkin(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="BandanaMapConverter", CompactNodeTitle="BandanaMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapBandana(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);

	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="ScarfMapConverter", CompactNodeTitle="ScarfMapConverted"))
	static FInventoryEquippedItemData ConvertEquippedMapScarf(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
	
	UFUNCTION(BlueprintPure, Category = "InventoryConversion", meta=(DisplayName="CloakMapConverter", CompactNodeTitle="CloakMapConverter"))
	static FInventoryEquippedItemData ConvertEquippedMapCloak(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap);
	

private:	
	static FInventoryEquippedItemData GetItemFromEquippedMapByTagInternal(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap, const FGameplayTag& SlotTag);

};
