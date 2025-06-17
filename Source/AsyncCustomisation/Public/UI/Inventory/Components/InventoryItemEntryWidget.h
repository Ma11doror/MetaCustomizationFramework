#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/Inventory/Data/InventoryListItemData.h"
#include "UI/Inventory/Data/PaletteRequester.h"
#include "InventoryItemEntryWidget.generated.h"

enum class EItemSlot : uint8;
class UCustomizationAssetManager;
class UInventoryItemEntryData;

UCLASS()
class ASYNCCUSTOMISATION_API UInventoryItemEntryWidget : public UCommonButtonBase, public IUserObjectListEntry, public IPaletteRequester
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintImplementableEvent)
	void HandleFilterChanged(EItemSlot InFilterType);
	virtual void SetPaletteRequestHandler(const FOnRequestColorPalette& InDelegate) override;
	
protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeOnInitialized() override;
	
	FDelegateHandle FilterDelegateHandle;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCommonButtonBase> PaletteButton;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> CurrencyIcon;

	TWeakObjectPtr<UInventoryItemEntryData> EntryData = nullptr;
	TWeakObjectPtr<UCustomizationAssetManager> AssetManager = nullptr;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetIsEquippedVisualState(bool IsEquipped);

	void OnCustomizeButtonClicked() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UInventoryListItemData* InventoryListItemData;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateUI(UInventoryListItemData* InData);
	
private:
	FOnRequestColorPalette PaletteRequestHandler;
};
