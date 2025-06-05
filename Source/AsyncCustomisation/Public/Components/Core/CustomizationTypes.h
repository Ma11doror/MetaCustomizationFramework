#pragma once

#include "BodyPartTypes.h"
//#include "Utilities/CommonUtilities.h"
#include "CustomizationSlotTypes.h"
#include "Somatotypes.h"
#include "UObject/StrongObjectPtr.h"
#include "CustomizationTypes.generated.h"

struct FEquippedItemsInSlotInfo;

USTRUCT()
struct FEquippedItemActorsInfo
{
	GENERATED_BODY()

	FName ItemSlug = NAME_None;
	TArray<TWeakObjectPtr<AActor>> ItemRelatedActors;

	bool operator==(const FEquippedItemActorsInfo& Other) const { return this->ItemSlug == Other.ItemSlug; }
	bool operator!=(const FEquippedItemActorsInfo& Other) const { return !(*this == Other); }

	void Clear()
	{
		for (TWeakObjectPtr<AActor>& ActorPtr : ItemRelatedActors)
		{
			if (AActor* ValidActor = ActorPtr.Get())
			{
				if (IsValid(ValidActor) && !ValidActor->IsGarbageEliminationEnabled())
				{
					// UE_LOG(LogTemp, Log, TEXT("FCustomizationContextData::ClearAttachedActors: Destroying actor %s from item %s"), *ValidActor->GetName(), *ItemInfo.ItemSlug.ToString());
					ValidActor->Destroy();
				}
			}
		}
		ItemRelatedActors.Empty();
	}

	
	FString ToString() const
	{
		FString ActorNames;
		for (int32 i = 0; i < ItemRelatedActors.Num(); ++i)
		{
			if (ItemRelatedActors[i].IsValid() && ItemRelatedActors[i]->IsValidLowLevel())
			{
				ActorNames += ItemRelatedActors[i]->GetName();
				if (i < ItemRelatedActors.Num() - 1)
				{
					ActorNames += ", ";
				}
			}
			else
			{
				ActorNames += "Invalid Actor";
				if (i < ItemRelatedActors.Num() - 1)
				{
					ActorNames += ", ";
				}
			}
		}

		return FString::Printf(TEXT("ItemSlug: %s, Actors: [%s]"), *ItemSlug.ToString(), *ActorNames);
	}
};

bool operator==(const TMap<FName, EBodyPartType>& LHS, const TMap<FName, EBodyPartType>& RHS);
bool operator!=(const TMap<FName, EBodyPartType>& LHS, const TMap<FName, EBodyPartType>& RHS);
bool operator==(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& Map, const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& RHS);
bool operator!=(const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& Map, const TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo>& RHS);

USTRUCT()
struct FEquippedItemsInSlotInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FEquippedItemActorsInfo> EquippedItemActors;

	bool operator==(const FEquippedItemsInSlotInfo& Other) const { return this->EquippedItemActors == Other.EquippedItemActors; }
	bool operator!=(const FEquippedItemsInSlotInfo& Other) const { return !(*this == Other); }

	void Clear()
	{
		for (auto& Iter : EquippedItemActors)
		{
			Iter.Clear();
		}
	}
	
	FString ToString() const
	{
		FString Result = "Equipped Items:\n";
		for (int32 i = 0; i < EquippedItemActors.Num(); ++i)
		{
			Result += FString::Printf(TEXT("  Item %d: %s\n"), i + 1, *EquippedItemActors[i].ToString());
		}
		return Result;
	}
};

USTRUCT()
struct FCharacterVFXCustomization
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> CustomMaterial = nullptr;
	
	bool operator==(const FCharacterVFXCustomization& Other) const
	{
		return CustomMaterial == Other.CustomMaterial;
	}

	
	bool operator!=(const FCharacterVFXCustomization& Other) const
	{
		return !(*this == Other);
	}
	
	FString ToString() const
	{
		if (CustomMaterial.IsNull())
		{
			return "CustomMaterial: None";
		}
		else
		{
			FString AssetPath = CustomMaterial.ToSoftObjectPath().ToString();
			return FString::Printf(TEXT("CustomMaterial: %s"), *AssetPath);
		}
	}
};

USTRUCT(Blueprintable)
struct FCustomizationContextData
{
	/*
	 * Always keep UCustomizationInvalidationContext::Update in actual state when change this Struct
	 */
	GENERATED_BODY()

public:
	// [Body] Type of body. Base type of skeletal
	ESomatotype Somatotype = ESomatotype::None;
	
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

	void ClearAttachedActors()
	{
		UE_LOG(LogTemp, Warning, TEXT("FCustomizationContextData::ClearAttachedActors - Clearing %d slots."), EquippedCustomizationItemActors.Num());
		for (auto& SlotPair : EquippedCustomizationItemActors)
		{
			UE_LOG(LogTemp, Warning, TEXT("FCustomizationContextData::ClearAttachedActors - Clearing slot %s."), *UEnum::GetValueAsString(SlotPair.Key));
			SlotPair.Value.Clear();
		}

		 EquippedCustomizationItemActors.Empty();
	}
	
	TArray<FName> GetEquippedSlugs() const
	{
		TArray<FName> Slugs;

		// Add material slugs from EquippedMaterialsMap
		for (const auto& [MaterialSlug, BodyPartType] : EquippedMaterialsMap)
		{
			Slugs.Add(MaterialSlug);
		}

		// Add body part slugs
		TArray<FName> ItemBodyPartSlugs;
		EquippedBodyPartsItems.GenerateKeyArray(ItemBodyPartSlugs);
		Slugs.Append(MoveTempIfPossible(ItemBodyPartSlugs));

		// Add item slugs from EquippedCustomizationItemActors
		for (const auto& [SlotType, ActorsInSlot] : EquippedCustomizationItemActors)
		{
			for (const auto& ActorsInfo : ActorsInSlot.EquippedItemActors)
			{
				Slugs.Add(ActorsInfo.ItemSlug);
			}
		}

		return Slugs;
	}

	bool operator==(const FCustomizationContextData& Other) const
	{
		return Somatotype == Other.Somatotype &&
			   EquippedMaterialsMap == Other.EquippedMaterialsMap &&
			   EquippedBodyPartsItems == Other.EquippedBodyPartsItems &&
			   EquippedCustomizationItemActors == Other.EquippedCustomizationItemActors &&
			   SkinVisibilityFlags == Other.SkinVisibilityFlags && // Assumes FSkinFlagCombination has operator==
			   VFXCustomization == Other.VFXCustomization;
	}

	bool operator!=(const FCustomizationContextData& Other) const
	{
		return !(*this == Other);
	}

	
	void ReplaceOrAddSpawnedActors(const FName& InItemSlug, ECustomizationSlotType InSlotType, const TArray<TWeakObjectPtr<AActor>>& InActors)
	{
		FEquippedItemsInSlotInfo& SlotInfo = EquippedCustomizationItemActors.FindOrAdd(InSlotType);
		FEquippedItemActorsInfo* FoundItem = SlotInfo.EquippedItemActors.FindByPredicate(
			[&InItemSlug](const FEquippedItemActorsInfo& Info){ return Info.ItemSlug == InItemSlug; });
		
		if (FoundItem)
		{
			FoundItem->ItemRelatedActors = InActors;
		}
		else
		{
			SlotInfo.EquippedItemActors.Add({InItemSlug, InActors});
		}
	}

	// TODO:: template function for text getters
	FString GetActorsList() const
	{
		FString ActorsList;
		for (auto Actor : EquippedCustomizationItemActors)
		{
			ActorsList += UEnum::GetValueAsString(Actor.Key) + Actor.Value.ToString() + TEXT(" \n");
		}
		return ActorsList;
	}

	FString GetItemsList() const
	{
		FString IterList;
		for (auto Iter : EquippedBodyPartsItems)
		{
			IterList.Append(Iter.Key.ToString() + UEnum::GetValueAsString(Iter.Value)  + TEXT(" \n"));
		}
		return IterList;
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
			UE_LOG(LogTemp, Log, TEXT("CalculateReason: Somatotype changed, Reason is All."));
		}
		if (IfAny([](auto& Any) { return Any.EquippedMaterialsMap.Num() > 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Skin);
			UE_LOG(LogTemp, Log, TEXT("CalculateReason: Materials changed, Added Skin flag."));
		}
		if (IfAny([](auto& Any) { return Any.EquippedBodyPartsItems.Num() > 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Body);
			UE_LOG(LogTemp, Log, TEXT("CalculateReason: BodyParts changed, Added Body flag."));

			// Remove flag because we call SkinInvalidation after Body invalidation
			EnumRemoveFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		if (IfAny([](auto& Any) { return Any.EquippedCustomizationItemActors.Num() > 0; }))
		{
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Actors);
			UE_LOG(LogTemp, Log, TEXT("CalculateReason: Actors changed, Added Actors flag."));
			// Invalidate BodyParts because maybe we need occure another variant instead current
			EnumAddFlags(Reason, ECustomizationInvalidationReason::Body);

			// Remove flag because we call SkinInvalidation after Actors invalidation
			EnumRemoveFlags(Reason, ECustomizationInvalidationReason::Skin);
		}
		return Reason;
	}
	
void CheckDiff(const FCustomizationContextData& NewState, const FCustomizationContextData& OldStateToCompareAgainst)
{
	Added = FCustomizationContextData();
    Removed = FCustomizationContextData();
    
    UE_LOG(LogTemp, Log, TEXT("CheckDiff: START. OldState Somato: %s, NewState Somato: %s"), 
        *UEnum::GetValueAsString(OldStateToCompareAgainst.Somatotype), 
        *UEnum::GetValueAsString(NewState.Somatotype));
    UE_LOG(LogTemp, Log, TEXT("CheckDiff: OldState BodyParts Num: %d, NewState BodyParts Num: %d"), 
        OldStateToCompareAgainst.EquippedBodyPartsItems.Num(), 
        NewState.EquippedBodyPartsItems.Num());

    TSet<FName> OldSlugs, NewSlugs;
    for (const auto& Pair : OldStateToCompareAgainst.EquippedBodyPartsItems) OldSlugs.Add(Pair.Key);
    for (const auto& Pair : NewState.EquippedBodyPartsItems) NewSlugs.Add(Pair.Key);

    TSet<FName> AddedSlugsSet = NewSlugs.Difference(OldSlugs);
    for (const FName& Slug : AddedSlugsSet)
    {
        Added.EquippedBodyPartsItems.Add(Slug, NewState.EquippedBodyPartsItems.FindChecked(Slug));
        UE_LOG(LogTemp, Warning, TEXT("CheckDiff: Added BodyPart Slug: %s. Type: %s"), *Slug.ToString(), *UEnum::GetValueAsString(NewState.EquippedBodyPartsItems.FindChecked(Slug)));
    }

    TSet<FName> RemovedSlugsSet = OldSlugs.Difference(NewSlugs);
    for (const FName& Slug : RemovedSlugsSet)
    {
        Removed.EquippedBodyPartsItems.Add(Slug, OldStateToCompareAgainst.EquippedBodyPartsItems.FindChecked(Slug)); 
        UE_LOG(LogTemp, Warning, TEXT("CheckDiff: Removed BodyPart Slug: %s. Type: %s"), *Slug.ToString(), *UEnum::GetValueAsString(OldStateToCompareAgainst.EquippedBodyPartsItems.FindChecked(Slug)));
    }
		
    if (OldStateToCompareAgainst.EquippedCustomizationItemActors != NewState.EquippedCustomizationItemActors)
    {
        TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo> AddedActors;
        TMap<ECustomizationSlotType, FEquippedItemsInSlotInfo> RemovedActors;

        for (const auto& NewPair : NewState.EquippedCustomizationItemActors)
        {
            const FEquippedItemsInSlotInfo* OldSlotInfo = OldStateToCompareAgainst.EquippedCustomizationItemActors.Find(NewPair.Key);
            if (!OldSlotInfo)
            {
                AddedActors.Add(NewPair.Key, NewPair.Value);
            }
            else 
            {
                FEquippedItemsInSlotInfo TempAddedItems;
                for(const auto& NewItemActorInfo : NewPair.Value.EquippedItemActors)
                {
                    if(!OldSlotInfo->EquippedItemActors.ContainsByPredicate(
                        [&](const FEquippedItemActorsInfo& OldItem){ return OldItem.ItemSlug == NewItemActorInfo.ItemSlug; }))
                    {
                        TempAddedItems.EquippedItemActors.Add(NewItemActorInfo);
                    }
                }
                if(TempAddedItems.EquippedItemActors.Num() > 0) AddedActors.Add(NewPair.Key, TempAddedItems);
            }
        }
        for (const auto& OldPair : OldStateToCompareAgainst.EquippedCustomizationItemActors)
        {
            const FEquippedItemsInSlotInfo* NewSlotInfo = NewState.EquippedCustomizationItemActors.Find(OldPair.Key);
            if (!NewSlotInfo)
            {
                RemovedActors.Add(OldPair.Key, OldPair.Value);
            }
            else
            {
                FEquippedItemsInSlotInfo TempRemovedItems;
                for(const auto& OldItemActorInfo : OldPair.Value.EquippedItemActors)
                {
                     if(!NewSlotInfo->EquippedItemActors.ContainsByPredicate(
                        [&](const FEquippedItemActorsInfo& NewItem){ return NewItem.ItemSlug == OldItemActorInfo.ItemSlug; }))
                    {
                        TempRemovedItems.EquippedItemActors.Add(OldItemActorInfo);
                    }
                }
                if(TempRemovedItems.EquippedItemActors.Num() > 0) RemovedActors.Add(OldPair.Key, TempRemovedItems);
            }
        }
        if(AddedActors.Num() > 0) Added.EquippedCustomizationItemActors = AddedActors;
        if(RemovedActors.Num() > 0) Removed.EquippedCustomizationItemActors = RemovedActors;

        if (AddedActors.Num() > 0 || RemovedActors.Num() > 0) {
             UE_LOG(LogTemp, Log, TEXT("CheckDiff: EquippedCustomizationItemActors changed. Added maps: %d, Removed maps: %d"), AddedActors.Num(), RemovedActors.Num());
        }
    }
		
    if (OldStateToCompareAgainst.EquippedMaterialsMap != NewState.EquippedMaterialsMap)
    {
        for(const auto& NewMatPair : NewState.EquippedMaterialsMap) {
            if(!OldStateToCompareAgainst.EquippedMaterialsMap.Contains(NewMatPair.Key)) {
                Added.EquippedMaterialsMap.Add(NewMatPair.Key, NewMatPair.Value);
            }
        }
        for(const auto& OldMatPair : OldStateToCompareAgainst.EquippedMaterialsMap) {
            if(!NewState.EquippedMaterialsMap.Contains(OldMatPair.Key)) {
                Removed.EquippedMaterialsMap.Add(OldMatPair.Key, OldMatPair.Value);
            }
        }
        if(Added.EquippedMaterialsMap.Num() > 0 || Removed.EquippedMaterialsMap.Num() > 0) {
            UE_LOG(LogTemp, Log, TEXT("CheckDiff: EquippedMaterialsMap changed. Added: %d, Removed: %d"), Added.EquippedMaterialsMap.Num(), Removed.EquippedMaterialsMap.Num());
        }
    }

    // Somatotype
    if (OldStateToCompareAgainst.Somatotype != NewState.Somatotype)
    {
        if (NewState.Somatotype != ESomatotype::None) Added.Somatotype = NewState.Somatotype;
        if (OldStateToCompareAgainst.Somatotype != ESomatotype::None) Removed.Somatotype = OldStateToCompareAgainst.Somatotype; 
        UE_LOG(LogTemp, Log, TEXT("CheckDiff: Somatotype changed from %s to %s."), *UEnum::GetValueAsString(OldStateToCompareAgainst.Somatotype), *UEnum::GetValueAsString(NewState.Somatotype));
    }

    this->Current = NewState; 
    UE_LOG(LogTemp, Log, TEXT("CheckDiff: END. InvalidationContext.Current updated to NewState."));
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
