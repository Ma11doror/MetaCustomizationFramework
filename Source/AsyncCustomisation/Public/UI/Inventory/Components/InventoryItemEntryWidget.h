#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "InventoryItemEntryWidget.generated.h"

enum class EItemType : uint8;
class UCustomizationAssetManager;
class UInventoryItemEntryData;

UCLASS()
class ASYNCCUSTOMISATION_API UInventoryItemEntryWidget : public UCommonButtonBase, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintImplementableEvent)
	void HandleFilterChanged(EItemType InFilterType);
	
protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnEntryReleased() override;
	
	FDelegateHandle FilterDelegateHandle;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> CurrencyIcon;

	TWeakObjectPtr<UInventoryItemEntryData> EntryData = nullptr;
	TWeakObjectPtr<UCustomizationAssetManager> AssetManager = nullptr;
};
