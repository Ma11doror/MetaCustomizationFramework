#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Components/CustomizationComponent.h"
#include "ItemSlotWidget.generated.h"

struct FInventoryEquippedItemData;
enum class EItemType : uint8;
enum class EItemTier : uint8;

UCLASS()
class ASYNCCUSTOMISATION_API UItemSlotWidget : public UCommonButtonBase
{
    GENERATED_BODY()

    
public:
    UItemSlotWidget(const FObjectInitializer& ObjectInitializer);
    

    UFUNCTION(BlueprintCallable)
    void SetSlotData(const FInventoryEquippedItemData& InData);

    EItemType GetSlotType() const
    {
        return ItemType;
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
    void SetItemNameText(const FText& InItemName);

protected:
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta= (ExposeOnSpawn = true))
    FText DefaultSlotName = FText::FromString("None");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta= (ExposeOnSpawn = true))
    UTexture2D* DefaultSlotIcon = nullptr;
    
    UPROPERTY(BlueprintReadWrite)
    FText DefaultItemName = FText::FromString("Nothing Equipped");

    UFUNCTION(BlueprintImplementableEvent)
    void ActivateVisualEffects();

    void SetDefaults();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    EItemType ItemType = EItemType::None;
    
};

