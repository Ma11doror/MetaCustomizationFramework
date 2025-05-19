#pragma once

#include "CoreMinimal.h"
#include "AsyncCustomisation/Public/Utilities/CounterComponent.h"
#include "AsyncCustomisation/Public/Utilities/TimerComponent.h"
#include "Core/CharacterComponentBase.h"
#include "Core/CustomizationTypes.h"

#include "Core/BodyPartTypes.h"

#include "CustomizationComponent.generated.h"

struct FBodyPartVariant;
class UCustomizationAssetManager;
class USomatotypeDataAsset;
DECLARE_LOG_CATEGORY_EXTERN(LogCustomizationComponent, Log, All);

enum class EItemType : uint8;
class UBodyPartAsset;
class UMaterialCustomizationDataAsset;
class UMaterialPackCustomizationDA;

UCLASS()
class ASYNCCUSTOMISATION_API UCustomizationComponent : public UCharacterComponentBase
{
	GENERATED_BODY()

public:
	UCustomizationComponent();
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSomatotypeLoaded, USomatotypeDataAsset* )
	FOnSomatotypeLoaded OnSomatotypeLoaded;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewCustomizationActorsAttached, const TArray<AActor*>& /* NewAttachedActors */)
	FOnNewCustomizationActorsAttached OnNewCustomizationActorsAttached;

	TMulticastDelegate<void(ABaseCharacter* /*InCharacter*/)> OnInvalidationPipelineCompleted;

	void UpdateFromOwning();

	void EquipItem(const FName& ItemSlug);
	void EquipItems(const TArray<FName>& Items);
	void UnequipItem(const FName& ItemSlug);
	bool RequestUnequipSlot(ECustomizationSlotType InSlotToUnequip);
	
	void ResetAll();
	void HardRefreshAll();

	void SetCustomizationContext(const FCustomizationContextData& InContext);
	
	UFUNCTION(BlueprintPure, Category = "Customization")
	const FCustomizationContextData& GetCurrentCustomizationState();

	const TMap<EBodyPartType, USkeletalMeshComponent*>& GetSkeletals();

	void SetAttachedActorsSimulatePhysics(bool bSimulatePhysics);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemsChanged, const FCustomizationContextData&, NewState);
	UPROPERTY(BlueprintAssignable, Category = "Customization")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

protected:
	void ClearContext();

	//Invalidation
	void Invalidate(const FCustomizationContextData& TargetState,
	                const bool bDeffer = true,
	                ECustomizationInvalidationReason ExplicitReason = ECustomizationInvalidationReason::None);

	//Deffer Invalidation context
	ECustomizationInvalidationReason DefferReason = ECustomizationInvalidationReason::None;

	// 
	TOptional<FCustomizationContextData> DeferredTargetState;
	
	void OnDefferInvalidationTimerExpired();

	//Allowed only to be called in Invalidate(...) method or other Invalidate*
	void InvalidateSkin(FCustomizationContextData& TargetState);
	void InvalidateBodyParts(FCustomizationContextData& TargetState);
	void InvalidateAttachedActors(FCustomizationContextData& TargetState);

	void StartInvalidationTimer(const FCustomizationContextData& TargetState); 
	void CreateTimerIfNeeded();



	void ResetUnusedBodyParts(const FCustomizationContextData& TargetState, const TSet<EBodyPartType>& FinalUsedPartTypes);

	void ApplyBodySkin(const FCustomizationContextData& TargetState,
	                   const USomatotypeDataAsset* SomatotypeDataAsset,
	                   TSet<EBodyPartType>& FinalUsedPartTypes,
	                   TMap<FName, const FBodyPartVariant*> FinalSlugToVariantMap);

	void LoadAndProcessBodyParts(FCustomizationContextData& TargetState,
	                             USomatotypeDataAsset* SomatotypeDataAsset,
	                             TArray<FPrimaryAssetId>& AllRelevantItemAssetIds);

	TStrongObjectPtr<UTimerComponent> InvalidationTimer = nullptr;
	
	UPROPERTY()
	FCustomizationContextData CurrentCustomizationState;

	UPROPERTY() /* Contains diff for Invalidation and current state*/
	FCustomizationInvalidationContext InvalidationContext;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool OnlyOneItemInSlot = false;
	
	TMap<EBodyPartType, USkeletalMeshComponent*> Skeletals;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	FCounterComponent PendingInvalidationCounter;
private:
	struct FCustomizationDebugInfo
	{
		FCustomizationDebugInfo()
		{
			Reset();
		}

		void Reset()
		{
			EquippedItems = GLOBAL_CONSTANTS::NONE_STRING;
			PendingItems = GLOBAL_CONSTANTS::NONE_STRING;
			SkinInfo = GLOBAL_CONSTANTS::NONE_STRING;
			ActorInfo = GLOBAL_CONSTANTS::NONE_STRING;
			SkinCoverage = GLOBAL_CONSTANTS::NONE_STRING;
		}
		
		FString EquippedItems;
		FString PendingItems;
		FString SkinInfo;
		FString ActorInfo;
		FString SkinCoverage;

		static FString GetBlockText(const FString& Title, const FString& Content) 
		{
			FString BlockText = "-------------------\n";
			BlockText += Title + ":\n";
			BlockText += Content + "\n";
			BlockText += "-------------------";
			return BlockText;
		}

		static FString FormatData(const TMap<FName, EBodyPartType>& Items)
		{
			FString FormattedText;
			for (const auto& Pair : Items)
			{
				FormattedText += FString::Printf(TEXT("%s : %s\n"), *Pair.Key.ToString(), *UEnum::GetValueAsString(Pair.Value));
			}
			return FormattedText;
		}

		static FString FormatData(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& Items)
		{
			FString FormattedText;
			for (const auto& Pair : Items)
			{
				FormattedText += FString::Printf(TEXT("%s : %s\n"), *UEnum::GetValueAsString(Pair.Key), *Pair.Value.ToString());
			}
			return FormattedText;
		}
		
		static FString FormatData(FSkinFlagCombination& Flags)
		{
			return Flags.ToString();
		}
		
		FString FormatData(const FCharacterVFXCustomization& VFX) const
		{
			return VFX.ToString(); 
		}
		
		const float VerticalOffset = 100.0f; 
		const float HorizontalOffset = 100.0f;
		const float OffsetForSecondLine = 65.0f;
		
	};

	FCustomizationDebugInfo DebugInfo;
	FTimerHandle TimerHandle;

	void UpdateDebugInfo();
	void DrawDebugTextBlock(const FVector& Location, const FString& Text, AActor* OwningActor, const FColor& Color);
};