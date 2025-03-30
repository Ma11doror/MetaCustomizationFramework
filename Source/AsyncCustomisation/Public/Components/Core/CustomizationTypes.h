#pragma once

#include "BodyPartTypes.h"
//#include "Utilities/CommonUtilities.h"
#include "CustomizationSlotTypes.h"
#include "Somatotypes.h"
#include "UObject/StrongObjectPtr.h"
#include "CustomizationTypes.generated.h"

USTRUCT()
struct FEquippedItemActorsInfo
{
	GENERATED_BODY()

	FName ItemSlug = NAME_None;
	TArray<TStrongObjectPtr<AActor>> ItemRelatedActors = {};

	bool operator==(const FEquippedItemActorsInfo& Other) const { return this->ItemSlug == Other.ItemSlug; }
	bool operator!=(const FEquippedItemActorsInfo& Other) const { return !(*this == Other); }
};

USTRUCT()
struct FEquippedItemsInSlotInfo
{
	GENERATED_BODY()

	TArray<FEquippedItemActorsInfo> EquippedItemActors;

	bool operator==(const FEquippedItemsInSlotInfo& Other) const { return this->EquippedItemActors == Other.EquippedItemActors; }
	bool operator!=(const FEquippedItemsInSlotInfo& Other) const { return !(*this == Other); }
};

USTRUCT()
struct FCharacterVFXCustomization
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> CustomFeatherMaterial = nullptr;
};

USTRUCT()
struct FCustomizationContextData
{
	/*
	 * Always keep UCustomizationInvalidationContext::Update in actual state when change this Struct
	 */
	GENERATED_BODY()

public:
	// [Body] Type of body. Base type of skeletal
	ESomatotype Somatotype = ESomatotype::None;
	
	//[Material] Materials
	FName EquippedMaterialSlug = NAME_None;
	
	//[Materials by slot] <Slug, BodyTartType>
	TMap<FName, EBodyPartType> EquippedMaterialsMap{};
	
	//[Body] Mapping item slug on BodyPartType
	TMap<FName, EBodyPartType> EquippedBodyPartsItems = {};
	
	//[Actors] Mapping SlotType on item id and Attached actors
	TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo> EquippedCustomizationItemActors = {};
	
	//Skin visibility flag list
	FSkinFlagCombination SkinVisibilityFlags;
	
	//[Skin] VFX customization
	FCharacterVFXCustomization VFXCustomization = {};

	TArray<FName> GetEquippedSlugs()
	{
		TArray<FName> Slugs{};
		if (EquippedMaterialSlug != NAME_None)
		{
			Slugs.Add(EquippedMaterialSlug);
		}

		TArray<FName> ItemBodyPartSlugs;
		EquippedBodyPartsItems.GenerateKeyArray(ItemBodyPartSlugs);
		Slugs.Append(MoveTempIfPossible(ItemBodyPartSlugs));

		for (const auto& [SlotType, ActorsInSlot] : EquippedCustomizationItemActors)
		{
			for (const auto& ActorsInfo : ActorsInSlot.EquippedItemActors)
			{
				Slugs.Add(ActorsInfo.ItemSlug);
			}
		}

		return Slugs;
	}

	void ReplaceOrAddSpawnedActors(const FName& ItemSlug, ECustomizationSlotType SlotType, const TArray<TStrongObjectPtr<AActor>>& Actors)
	{
		TArray<FEquippedItemActorsInfo>& ActorsInSlot = EquippedCustomizationItemActors.FindOrAdd(SlotType).EquippedItemActors;
		if (FEquippedItemActorsInfo* ActorsInfo = ActorsInSlot.FindByPredicate(
				[&ItemSlug](const FEquippedItemActorsInfo& ActorsInfo) { return ActorsInfo.ItemSlug == ItemSlug; }))
		{
			ActorsInfo->ItemRelatedActors = Actors;
		}
		else
		{
			ActorsInSlot.Add({ ItemSlug, Actors });
		}
	}
};

UENUM()
enum class ECustomizationInvalidationReason : uint8
{
	None		= 0,
	/*Flags*/
	Body		= 1 << 0,
	Actors		= 1 << 1,
	Skin		= 1 << 2,
	/*Masks*/
	All			= Body | Actors | Skin
};
ENUM_CLASS_FLAGS(ECustomizationInvalidationReason);
ENUM_RANGE_BY_FIRST_AND_LAST(
	ECustomizationInvalidationReason, ECustomizationInvalidationReason::None, ECustomizationInvalidationReason::All)

bool operator==(const TMap<FName, EBodyPartType>& Map, const TMap<FName, EBodyPartType>& RHS);
bool operator!=(const TMap<FName, EBodyPartType>& LHS, const TMap<FName, EBodyPartType>& RHS);
bool operator==(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& Map, const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& RHS);
bool operator!=(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& Map, const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& RHS);


inline bool operator==(const TMap<FName, EBodyPartType>& LHS, const TMap<FName, EBodyPartType>& RHS)
{
	if (LHS.Num() != RHS.Num())
	{
		return false;
	}

	for (const auto& Pair : LHS)
	{
		const EBodyPartType* OtherValue = RHS.Find(Pair.Key);
		if (!OtherValue || *OtherValue != Pair.Value)
		{
			return false;
		}
	}

	return true;
}

inline bool operator!=(const TMap<FName, EBodyPartType>& LHS, const TMap<FName, EBodyPartType>& RHS)
{
	return !(LHS == RHS);
}

inline bool operator==(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& LHS, const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& RHS)
{
	if (LHS.Num() != RHS.Num())
	{
		return false;
	}

	for (const auto& Pair : LHS)
	{
		const FEquippedItemsInSlotInfo* OtherValue = RHS.Find(Pair.Key);
		if (!OtherValue || *OtherValue != Pair.Value)
		{
			return false;
		}
	}

	return true;
}

inline bool operator!=(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& LHS, const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& RHS)
{
	return !(LHS == RHS);
}

USTRUCT()
struct FCustomizationInvalidationContext
{
	GENERATED_BODY()

	/*
	 * Contains diff data what been collected during invalidation beginning and used in invalidation methods
	 */
public:
	// Temporal data
	UPROPERTY()
	FCustomizationContextData Added = {};
	UPROPERTY()
	FCustomizationContextData Removed = {};
	
	// Persistent data used to gather diff
	UPROPERTY()
	FCustomizationContextData Current = {};

	ECustomizationInvalidationReason CalculateReason()
	{
		auto IfAny = [&](auto Predicate) {
			for (const auto& Context : TArray { &Added, &Removed })
			{
				if (Predicate(*Context))
				{
					return true;
				}
			}
			return false;
		};

		ECustomizationInvalidationReason Reason = ECustomizationInvalidationReason::None;

		if (IfAny([](auto& Any) { return Any.Somatotype != ESomatotype::None; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::All);
		}
		if (IfAny([](auto& Any) { return Any.EquippedMaterialsMap.Num() > 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		if (IfAny([](auto& Any) { return Any.EquippedBodyPartsItems.Num() > 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Body);

			// Remove flag because we call SkinInvalidation after Body invalidation
			EnumRemoveFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		if (IfAny([](auto& Any) { return Any.EquippedCustomizationItemActors.Num() > 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Actors);

			// Invalidate BodyParts because maybe we need occure another variant instead current
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Body);

			// Remove flag because we call SkinInvalidation after Actors invalidation
			EnumRemoveFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		return Reason;
	}
	
void CheckDiff(const FCustomizationContextData& New)
{
	Added = FCustomizationContextData();
	Removed = FCustomizationContextData();

	// Compare EquippedBodyPartsItems using TSet for efficient comparison
	TSet<FName> CurrentSlugs;
	TSet<FName> NewSlugs;

	for (const auto& [Slug, PartType] : Current.EquippedBodyPartsItems)
	{
		CurrentSlugs.Add(Slug);
	}

	for (const auto& [Slug, PartType] : New.EquippedBodyPartsItems)
	{
		NewSlugs.Add(Slug);
	}

	// added slugs
	TSet<FName> AddedSlugs = NewSlugs.Difference(CurrentSlugs);
	for (const FName& Slug : AddedSlugs)
	{
		Added.EquippedBodyPartsItems.Add(Slug, New.EquippedBodyPartsItems.FindChecked(Slug));
		UE_LOG(LogTemp, Warning, TEXT("Added.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), *Slug.ToString(), *UEnum::GetValueAsString(New.EquippedBodyPartsItems.FindChecked(Slug)));
	}

	// removed slugs
	TSet<FName> RemovedSlugs = CurrentSlugs.Difference(NewSlugs);
	for (const FName& Slug : RemovedSlugs)
	{
		Removed.EquippedBodyPartsItems.Add(Slug, Current.EquippedBodyPartsItems.FindChecked(Slug));
		UE_LOG(LogTemp, Warning, TEXT("Removed.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), *Slug.ToString(), *UEnum::GetValueAsString(Current.EquippedBodyPartsItems.FindChecked(Slug)));
	}

	//  Customization actors 
	if (Current.EquippedCustomizationItemActors != New.EquippedCustomizationItemActors)
	{
		Added.EquippedCustomizationItemActors = New.EquippedCustomizationItemActors;
		Removed.EquippedCustomizationItemActors = Current.EquippedCustomizationItemActors;
	}

	// Materials Map
	if (Current.EquippedMaterialsMap != New.EquippedMaterialsMap)
	{
		Added.EquippedMaterialsMap = New.EquippedMaterialsMap;
		Removed.EquippedMaterialsMap = Current.EquippedMaterialsMap;
	}

	// Somatotype
	if (Current.Somatotype != New.Somatotype)
	{
		Added.Somatotype = New.Somatotype;
		Removed.Somatotype = Current.Somatotype;
	}

	//Update current
	Current = New;
}
	void ClearTemporaryContext()
	{
		Added = FCustomizationContextData();
		Removed = FCustomizationContextData();
	}

	void ClearAll()
	{
		ClearTemporaryContext();
		Current = FCustomizationContextData();
	}
};
