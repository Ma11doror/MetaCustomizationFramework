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
		if (IfAny([](auto& Any) { return Any.EquippedMaterialSlug != NAME_None; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		if (IfAny([](auto& Any) { return Any.EquippedBodyPartsItems.Num() != 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Body);

			// Remove flag because we call SkinInvalidation after Body invalidation
			EnumRemoveFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		if (IfAny([](auto& Any) { return Any.EquippedCustomizationItemActors.Num() != 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Actors);

			// Invalidate BodyParts cause we may need occure another variant instead current
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

		//// EquippedBodyPartsItems
		//for (const auto& [NewItemGuid, NewPartType] : New.EquippedBodyPartsItems)
		//{
		//	auto* OldGuid = Old.EquippedBodyPartsItems.FindKey(NewPartType);
		//	if (!OldGuid || *OldGuid != NewItemGuid)
		//	{
		//		UE_LOG(LogTemp, Warning, TEXT("Added.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), *NewItemGuid.ToString(), *UEnum::GetValueAsString(NewPartType));
		//		Added.EquippedBodyPartsItems.Add(NewItemGuid, NewPartType);
		//	}
		//}
		//for (const auto& [OldItemGuid, OldPartType] : Old.EquippedBodyPartsItems)
		//{
		//	auto* NewGuid = New.EquippedBodyPartsItems.FindKey(OldPartType);
		//
		//	if (!NewGuid || (*NewGuid != OldItemGuid))
		//	{
		//		UE_LOG(LogTemp, Warning, TEXT("Removed.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), *OldItemGuid.ToString(), *UEnum::GetValueAsString(OldPartType));
		//		Removed.EquippedBodyPartsItems.Add(OldItemGuid, OldPartType);
		//	}
		//}
		
		// EquippedBodyPartsItems - Compare by slug instead of by part type
		// Collect new body part items not in old
		for (const auto& [NewItemGuid, NewPartType] : New.EquippedBodyPartsItems)
		{
			if (!Current.EquippedBodyPartsItems.Contains(NewItemGuid))
			{
				UE_LOG(LogTemp, Warning, TEXT("Added.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), 
					*NewItemGuid.ToString(), *UEnum::GetValueAsString(NewPartType));
				Added.EquippedBodyPartsItems.Add(NewItemGuid, NewPartType);
			}
		}
		
		//for (const auto& [OldItemGuid, OldPartType] : Current.EquippedBodyPartsItems)
		//{
		//	if (!Added.EquippedBodyPartsItems.Contains(OldItemGuid))
		//	{
		//		//UE_LOG(LogTemp, Warning, TEXT("%s: Added to removed list."), ANSI_TO_TCHAR(__FUNCTION__));
		//		UE_LOG(LogTemp, Warning, TEXT("Removed.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), 
		//			*OldItemGuid.ToString(), *UEnum::GetValueAsString(OldPartType));
		//		Removed.EquippedBodyPartsItems.Add(OldItemGuid, OldPartType);
		//	}
		//}
		//// Collect old body part items not in new
		//for (const auto& [OldItemGuid, OldPartType] : Current.EquippedBodyPartsItems)
		//{
		//	if (!New.EquippedBodyPartsItems.Contains(OldItemGuid))
		//	{
		//		UE_LOG(LogTemp, Warning, TEXT("Removed.EquippedBodyPartsItems:: Added Item %s. BodyPartType: %s"), 
		//			*OldItemGuid.ToString(), *UEnum::GetValueAsString(OldPartType));
		//		Removed.EquippedBodyPartsItems.Add(OldItemGuid, OldPartType);
		//	}
		//}
		
		// EquippedCustomizationItemActors
		for (const auto& [NewSlotType, NewItemActors] : New.EquippedCustomizationItemActors)
		{
			auto* OldItemActors = Current.EquippedCustomizationItemActors.Find(NewSlotType);
			if (!OldItemActors || *OldItemActors != NewItemActors)
			{
				Added.EquippedCustomizationItemActors.Add(NewSlotType, NewItemActors);
			}
		}
		for (const auto& [OldSlotType, OldItemActors] : Current.EquippedCustomizationItemActors)
		{
			auto* NewItemActors = New.EquippedCustomizationItemActors.Find(OldSlotType);
			if (!NewItemActors || (*NewItemActors != OldItemActors))
			{
				Removed.EquippedCustomizationItemActors.Add(OldSlotType, OldItemActors);
			}
		}

		// EquippedSkinId
		if (Current.EquippedMaterialSlug != New.EquippedMaterialSlug)
		{
			Added.EquippedMaterialSlug = New.EquippedMaterialSlug;
			Removed.EquippedMaterialSlug = Current.EquippedMaterialSlug;
		}
		if (Current.Somatotype != New.Somatotype)
		{
			Added.Somatotype = New.Somatotype;
			Removed.Somatotype = Current.Somatotype;
		}

		//Update current
		//Current = New;
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
