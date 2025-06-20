#pragma once

#include "CoreMinimal.h"
#include "UI/ActivatableWidget.h"
#include "CommonUI/Public/Groups/CommonButtonGroupBase.h"
#include "Data/PaletteRequester.h"


#include "InventoryWidget.generated.h"

class UInventoryListItemData;
class APlayerControllerBase;
class UGameInstanceBase;
class UInventoryComponent;
class UCustomizationComponent;
class UVerticalBox;
class UHorizontalBox;
class UOverlay;
enum class EItemSlot : uint8;
class UWidgetSwitcher;
class UVM_Inventory;
class UUniformGridPanel;
class UImage;
class UTextBlock;
class UButton;
class UItemSlotWidget;
class UCommonAnimatedSwitcher;
class UCommonListView;
class UCommonTileView;

UENUM(BlueprintType)
enum class EInventorySwitcherTab : uint8
{
    SlotsView     UMETA(DisplayName = "Slots View"),
    ItemsListView UMETA(DisplayName = "Items List View")
};

UCLASS()
class ASYNCCUSTOMISATION_API UInventoryWidget : public UActivatableWidget
{
    GENERATED_BODY()

public:
    
    virtual void NativeDestruct() override;

    virtual void NativeOnInitialized() override;

    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ClearFilter();

    void SetActiveInventoryTab(EInventorySwitcherTab Tab);

    UFUNCTION(BlueprintCallable)
    void FinalizePaletteClosure();
    
    UFUNCTION(BlueprintCallable)
    void FinalizePaletteOpening(const FName& InItemSlug);
    
   // UPROPERTY(BlueprintReadWrite, Category="Inventory|Actions")
    FOnRequestColorPalette OnRequestColorPalette;

protected:
    UFUNCTION()
    void OnItemSlotClicked(UCommonButtonBase* AssociatedButton, int32 ButtonIndex);

    void ConstructButtons();
    
    UFUNCTION(BlueprintCallable)
    void OnListItemClicked(UObject* ItemObject);

    void OnBackButtonClicked();

    UFUNCTION(BlueprintImplementableEvent)
    void PlayClosePaletteAnimation();
    
    UFUNCTION(BlueprintImplementableEvent)
    void PlayOpenPaletteAnimation(const FName& InItemSlug);
    
    UFUNCTION(BlueprintImplementableEvent)
    void PlayFailEquipAnimation();
    
    void OnListTabOpened();
    
    UFUNCTION()
    void HandleRequestColorPalette(FName ItemSlug);
    void OnMainListItemObjectSet(UUserWidget& InEntryWidget);
 //   void OnItemIsHoveredChanged(UObject* InEntryWidget, bool InIsHovered);

    void OnPaletteItemClicked(UObject* Item);

    UPROPERTY()
    APlayerControllerBase* PlayerController;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (BindWidget))
    TObjectPtr<UCommonAnimatedSwitcher> InventorySwitcher = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (BindWidget))
    TObjectPtr<UOverlay> SlotsList = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (BindWidget))
    TObjectPtr<UOverlay> ItemsForSlotList = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (BindWidget))
    TObjectPtr<UCommonListView> ItemsList = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="MVVM")
    TObjectPtr<UVM_Inventory> InventoryViewModel = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (BindWidget))
    TObjectPtr<UCommonButtonBase> BackButton = nullptr;
    
    UPROPERTY()
    UCommonButtonGroupBase* InventorySlotsButtonGroup = nullptr;

    // tabs or slots
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCommonButtonBase* PantsButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCommonButtonBase* HatButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCommonButtonBase* WristButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCommonButtonBase* BodyButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCommonButtonBase* FeetButton;
    
    // Two Sections for our current design
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* LeftTabsBox;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* RightTabsBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (BindWidget))
    TObjectPtr<UOverlay> EmptyListPlaceholder = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Palette", meta = (BindWidget))
    TObjectPtr<UCommonTileView> SkinPaletteListView = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Palette", meta = (BindWidget))
    TObjectPtr<UWidget> SkinPaletteLoadingIndicator = nullptr;
};