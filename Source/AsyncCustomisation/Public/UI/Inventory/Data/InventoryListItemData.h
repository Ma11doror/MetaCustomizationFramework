#pragma once

#include "CoreMinimal.h"
#include "Components/Core/Data.h"
#include "Components/Core/Assets/ItemMetaAsset.h"
#include "UObject/Object.h"
#include "InventoryListItemData.generated.h" 

class UItemMetaAsset;
enum class EItemSlot : uint8;
enum class EItemTier : uint8;

DECLARE_DELEGATE_OneParam(FOnIsEquippedChanged, bool /*InIsEquipped*/);
DECLARE_MULTICAST_DELEGATE(FOnItemDataChanged);

UCLASS(BlueprintType, Abstract, DefaultToInstanced)
class ASYNCCUSTOMISATION_API UBaseListItemData : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Inventory Item")
	bool GetIsEquipped() const { return IsEquipped; }
	
	virtual void SetIsEquipped(bool bNewValue)
	{
		IsEquipped = bNewValue;
		OnIsEquippedChanged.ExecuteIfBound(IsEquipped);
		OnItemDataChanged.Broadcast();
	}
		
	FOnItemDataChanged OnItemDataChanged;
	
	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FPrimaryAssetId ItemId; 

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FName ItemSlug;         
	
	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FName Name;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	EItemTier Tier = EItemTier::Common;

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	EItemSlot ItemSlot = EItemSlot::None; 

	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	EItemType ItemType = EItemType::None;
	
	UPROPERTY(BlueprintReadOnly, Category="Inventory Item")
	bool IsEquipped = false;

	virtual void InitializeFromMeta(UItemMetaAsset* MetaAsset, int32 InitialCount, bool bIsCurrentlyEquipped)
	{
		if (!MetaAsset) return;

		ItemId = MetaAsset->GetPrimaryAssetId();
		ItemSlug = MetaAsset->GetFName();
		Name = MetaAsset->Name;
		Description = MetaAsset->Description;
		Icon = MetaAsset->Icon;
		Tier = MetaAsset->ItemTier;
		ItemSlot = MetaAsset->ItemSlot;
		ItemType = MetaAsset->ItemType;
		
		IsEquipped = bIsCurrentlyEquipped;
	}

	bool operator==(const UBaseListItemData& Other) const
	{
        
		return ItemId == Other.ItemId &&
			   ItemSlug == Other.ItemSlug &&
			   	Tier == Other.Tier &&  
		//	   Count == Other.Count &&
			   IsEquipped == Other.IsEquipped;
		// return ItemId == Other.ItemId && Count == Other.Count && bIsEquipped == Other.bIsEquipped;

		/*
		 *
		 *  Name.ToString() == Other.Name.ToString() &&
		 * 
		 *  ItemType == Other.ItemType &&
		 *  SlotType == Other.SlotType && 
		 */
	}
	
	FOnIsEquippedChanged OnIsEquippedChanged;
};


UCLASS(BlueprintType, DefaultToInstanced)
class ASYNCCUSTOMISATION_API UInventoryListItemData : public UBaseListItemData
{
	GENERATED_BODY()
public:
	virtual void InitializeFromMeta(UItemMetaAsset* MetaAsset, int32 InitialCount, bool bIsCurrentlyEquipped) override
	{
		Super::InitializeFromMeta(MetaAsset, InitialCount, bIsCurrentlyEquipped);
		HasSkins = MetaAsset->AvailableSkinAssetIds.Num() > 0;
		Count = InitialCount;
	}
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	int32 Count;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item")
	bool HasSkins;
	
	using SubscribeFuncType = TFunction<FDelegateHandle(UObject* /*SubscriberWidget*/, TFunction<void(EItemSlot)> /*WidgetHandler*/)>;
	using UnsubscribeFuncType = TFunction<void(FDelegateHandle)>;

	void SetEventSubscriptionFunctions(SubscribeFuncType InSubscribeFunc, UnsubscribeFuncType InUnsubscribeFunc)
	{
		SubscribeFunction = MoveTemp(InSubscribeFunc);
		UnsubscribeFunction = MoveTemp(InUnsubscribeFunc);
	}
	
	FDelegateHandle SubscribeToFilterChanges(UObject* SubscriberWidget, TFunction<void(EItemSlot)> WidgetHandler)
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
	// TODO:: need this maybe?
	// bool operator==(const UInventoryListItemData& Other) const
	// {
	// 	// Compare relevant fields. Base IsSameItem can be used if it compares enough.
	// 	return Super::IsSameItem(Other) && 
	// 		   Count == Other.Count;
	// }

private:
	SubscribeFuncType SubscribeFunction;
	UnsubscribeFuncType UnsubscribeFunction;
};

UCLASS(BlueprintType, DefaultToInstanced)
class ASYNCCUSTOMISATION_API USkinListItemData : public UBaseListItemData
{
	GENERATED_BODY()

public:
	virtual void InitializeFromMeta(UItemMetaAsset* MetaAsset, int32 InitialCount, bool bIsCurrentlyEquipped) override
	{
		Super::InitializeFromMeta(MetaAsset, InitialCount, bIsCurrentlyEquipped);

		if (const UItemShaderMetaAsset* ShaderMeta = Cast<UItemShaderMetaAsset>(MetaAsset))
		{
			ColorTint = ShaderMeta->ColorTint;
		}
		else
		{
			ColorTint = FLinearColor::Black;
		}
	}

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool bIsOwned = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Shader Item")
	FLinearColor ColorTint;
};