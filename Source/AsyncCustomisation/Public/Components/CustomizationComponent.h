#pragma once

#include "CoreMinimal.h"
#include "AsyncCustomisation/Public/Utilities/CounterComponent.h"
#include "AsyncCustomisation/Public/Utilities/TimerComponent.h"
#include "Core/CharacterComponentBase.h"
#include "Core/CustomizationTypes.h"

#include "Core/BodyPartTypes.h"

#include "Utilities/CounterComponent.h"
#include "Utilities/TimerComponent.h"
#include "CustomizationComponent.generated.h"

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
	
	void ResetAll();
	void HardRefreshAll();

	void SetCustomizationContext(const FCustomizationContextData& InContext);
	const FCustomizationContextData& GetCustomizationContext();

	const TMap<EBodyPartType, USkeletalMeshComponent*>& GetSkeletals();

	void SetAttachedActorsSimulatePhysics(bool bSimulatePhysics);

	UFUNCTION(BlueprintPure)
	ABaseCharacter* GetOwningCharacter();

protected:
	void ClearContext();

	//Invalidation
	void Invalidate(const bool bDeffer = true, ECustomizationInvalidationReason Reason = ECustomizationInvalidationReason::None );

	//Deffer Invalidation context
	ECustomizationInvalidationReason DefferReason = ECustomizationInvalidationReason::None;
	
	void OnDefferInvalidationTimerExpired();

	//Allowed only to be called in Invalidate(...) method or other Invalidate*
	void InvalidateSkin();
	void InvalidateBodyParts();
	void InvalidateAttachedActors();

	void StartInvalidationTimer();
	void CreateTimerIfNeeded();
	
	TStrongObjectPtr<UTimerComponent> InvalidationTimer = nullptr;
	
	FCustomizationContextData CollectedNewContextData;

	UPROPERTY() /* Contains diff for Invalidation and current state*/
	FCustomizationInvalidationContext InvalidationContext;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	bool OnlyOneItemInSlot = true;
	
	TMap<EBodyPartType, USkeletalMeshComponent*> Skeletals;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	FCounterComponent PendingInvalidationCounter;

	//UPROPERTY()
	//TWeakObjectPtr<AExampleCharacter> OwningCharacter;
};