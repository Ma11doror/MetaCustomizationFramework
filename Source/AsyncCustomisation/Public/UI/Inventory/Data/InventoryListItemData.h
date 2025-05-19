#pragma once

#include "CoreMinimal.h"
#include "Components/Core/Data.h"
#include "Components/Core/Assets/ItemMetaAsset.h"
#include "UObject/Object.h"
#include "InventoryListItemData.generated.h" 

class UItemMetaAsset;
enum class EItemType : uint8;
enum class EItemTier : uint8;

UCLASS(BlueprintType, DefaultToInstanced)
class ASYNCCUSTOMISATION_API UInventoryListItemData final : public UObject
{
	GENERATED_BODY()
	

public:
	using SubscribeFuncType = TFunction<FDelegateHandle(UObject* /*SubscriberWidget*/, TFunction<void(EItemType)> /*WidgetHandler*/)>;
	using UnsubscribeFuncType = TFunction<void(FDelegateHandle)>;

	void SetEventSubscriptionFunctions(SubscribeFuncType InSubscribeFunc, UnsubscribeFuncType InUnsubscribeFunc)
	{
		SubscribeFunction = MoveTemp(InSubscribeFunc);
		UnsubscribeFunction = MoveTemp(InUnsubscribeFunc);
	}
	
	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FPrimaryAssetId ItemId; 

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FName ItemSlug;         

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	int32 Count;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	EItemTier Tier = EItemTier::Common;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	EItemType ItemType = EItemType::None; 

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	bool bIsEquipped = false;
	
	void InitializeFromMeta(UItemMetaAsset* MetaAsset, int32 InitialCount, bool bIsCurrentlyEquipped)
	{
		if (!MetaAsset) return;

		ItemId = MetaAsset->GetPrimaryAssetId();
		ItemSlug = MetaAsset->GetFName();
		Name = MetaAsset->Name;
		Description = MetaAsset->Description;
		Icon = MetaAsset->Icon;
		Tier = MetaAsset->ItemTier;
		ItemType = MetaAsset->ItemType;
		Count = InitialCount;
		bIsEquipped = bIsCurrentlyEquipped;
	}

	FDelegateHandle SubscribeToFilterChanges(UObject* SubscriberWidget, TFunction<void(EItemType)> WidgetHandler)
	{
		if (SubscribeFunction)
		{
			return SubscribeFunction(SubscriberWidget, MoveTemp(WidgetHandler));
		}
		return FDelegateHandle();
	}

	void UnsubscribeFromFilterChanges(FDelegateHandle Handle)
	{
		if (UnsubscribeFunction && Handle.IsValid())
		{
			UnsubscribeFunction(Handle);
		}
	}
	
	bool operator==(const UInventoryListItemData& Other) const
	{
        
		return ItemId == Other.ItemId &&
			   ItemSlug == Other.ItemSlug && 
			   Count == Other.Count &&
			   bIsEquipped == Other.bIsEquipped;
		// return ItemId == Other.ItemId && Count == Other.Count && bIsEquipped == Other.bIsEquipped;

		/*
		 *
		 *  Name.ToString() == Other.Name.ToString() &&
		 *  Tier == Other.Tier &&  
		 *  ItemType == Other.ItemType &&
		 *  SlotType == Other.SlotType && 
		 */
	}

private:
	SubscribeFuncType SubscribeFunction;
	UnsubscribeFuncType UnsubscribeFunction;
};