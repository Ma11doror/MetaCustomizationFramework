#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "GameplayTagContainer.h"
#include "Components/CustomizationComponent.h"
#include "ItemSlotWidget.generated.h"

struct FInventoryEquippedItemData;
enum class EItemSlot : uint8;
enum class EItemTier : uint8;

UCLASS()
class ASYNCCUSTOMISATION_API UItemSlotWidget : public UCommonButtonBase
{
    GENERATED_BODY()

    
public:
    UItemSlotWidget(const FObjectInitializer& ObjectInitializer);
    

    UFUNCTION(BlueprintCallable)
    void SetSlotData(const FInventoryEquippedItemData& InData);

    FGameplayTag GetItemTag() const
    {
        return AssignedUITag;
    }

protected:

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintImplementableEvent)
    void SetSlotNameText(const FText& InItemSlug);
    
    UFUNCTION(BlueprintImplementableEvent)
    void SetItemIcon(UTexture2D* InIcon);
    
    UFUNCTION(BlueprintImplementableEvent)
    void SetItemTier(EItemTier InTier);
    
public:
    UFUNCTION(BlueprintImplementableEvent)
    void SetItemNameText(const FName& InItemName);

protected:
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta= (ExposeOnSpawn = true))
    FText DefaultSlotName = FText::FromString("None");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta= (ExposeOnSpawn = true))
    UTexture2D* DefaultSlotIcon = nullptr;

    // TODO:: Add it to global constants?
    UPROPERTY(BlueprintReadWrite)
    FName DefaultItemName = TEXT("Nothing Equipped");

    UFUNCTION(BlueprintImplementableEvent)
    void ActivateVisualEffects();

    void SetDefaults();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (GameplayTagFilter = "Slot.UI"))
    FGameplayTag AssignedUITag;
    
};

