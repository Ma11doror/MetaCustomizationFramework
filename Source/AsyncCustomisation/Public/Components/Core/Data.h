#pragma once

#include "CoreMinimal.h"
#include "Constants/GlobalConstants.h"
#include "Data.generated.h"

UENUM(BlueprintType)
enum class EItemTier : uint8
{
	None = 0,
	Common = 1,
	Uncommon = 2,
	Rare = 3,
	Epic = 4,
	Royal = 5,
};

ENUM_RANGE_BY_FIRST_AND_LAST(EItemTier, EItemTier::None, EItemTier::Royal)

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None = 0,
	Body,
	Haircut,
	Skin,
	Ring,
	Weighting,
	Bandan,
	Chain,
};

ENUM_RANGE_BY_FIRST_AND_LAST(EItemType, EItemType::Body, EItemType::Chain)

// actually now it's skin coverage flag
UENUM(BlueprintType)
enum class ESkinVisibilityFlag : uint8
{
	None            = 0 UMETA(Hidden),
	Wrist           = 1,
	Forearm         = 2,
	Elbow           = 3,
	Shoulder        = 4,
	Torso			= 5,
	Pelvis			= 6,
	Hip				= 7,
	Knee            = 8,
	Ankle           = 9,
	Feet            = 10
};

ENUM_RANGE_BY_FIRST_AND_LAST(ESkinVisibilityFlag, ESkinVisibilityFlag::None, ESkinVisibilityFlag::Feet)


USTRUCT(BlueprintType)
struct FSkinFlagCombination
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Bitmask, BitmaskEnum="/Script/AsyncCustomisation.ESkinVisibilityFlag"))
	int32 FlagMask = 0;

	UPROPERTY()
	FString FlagDescription;

	FSkinFlagCombination() : FlagMask(0), FlagDescription(TEXT("None"))
	{
	}


	FSkinFlagCombination(ESkinVisibilityFlag InFlag) : FlagMask((int32)InFlag)
	{
		UpdateDescription();
	}

	// TODO:: move to common utils
	template <typename ValueType>
	const ValueType* GetMatch(const TMap<FSkinFlagCombination, ValueType>& Map, int32 FeaturesMask)
	{
		// 1. Exact match
		FSkinFlagCombination ExactFeatures;
		ExactFeatures.FlagMask = FeaturesMask;

		if (const ValueType* ExactMatch = Map.Find(ExactFeatures))
		{
			return ExactMatch;
		}

		// 2. If fail -- find max match
		const ValueType* BestMatch = nullptr;
		int32 MaxMatchingBits = -1;
		int32 MinExtraBits = INT_MAX;

		for (const auto& Pair : Map)
		{
			int32 EntryFlags = Pair.Key.FlagMask;
			int32 CommonBits = FeaturesMask & EntryFlags;

			int32 MatchingBitCount = 0;
			int32 ExtraBitCount = 0;
			int32 MissingBitCount = 0;

			for (int32 BitPos = 0; BitPos < 32; ++BitPos)
			{
				int32 BitMask = (1 << BitPos);

				if ((CommonBits & BitMask) != 0)
				{
					MatchingBitCount++;
				}
				else if ((EntryFlags & BitMask) != 0)
				{
					ExtraBitCount++;
				}
				else if ((FeaturesMask & BitMask) != 0)
				{
					MissingBitCount++;
				}
			}

			if (MatchingBitCount > MaxMatchingBits ||
				(MatchingBitCount == MaxMatchingBits && ExtraBitCount < MinExtraBits))
			{
				MaxMatchingBits = MatchingBitCount;
				MinExtraBits = ExtraBitCount;
				BestMatch = &Pair.Value;
			}
		}

		if (BestMatch != nullptr)
		{
			return BestMatch;
		}

		// 4. Default =(
		FSkinFlagCombination DefaultFeatures;
		DefaultFeatures.FlagMask = 0;

		if (const ValueType* DefaultMatch = Map.Find(DefaultFeatures))
		{
			return DefaultMatch;
		}

		if (Map.Num() > 0)
		{
			auto FirstElement = Map.CreateConstIterator();
			return &FirstElement->Value;
		}

		return nullptr;
	}

	bool operator==(const FSkinFlagCombination& Other) const
	{
		return FlagMask == Other.FlagMask;
	}

	friend uint32 GetTypeHash(const FSkinFlagCombination& FeatureCombo)
	{
		return ::GetTypeHash(FeatureCombo.FlagMask);
	}

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
	{
		if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FSkinFlagCombination, FlagMask))
		{
			UpdateDescription();
		}
	}

	const FString& UpdateDescription()
	{
		TArray<FString> ActiveFeatures;

		if ((FlagMask & (int32)ESkinVisibilityFlag::Wrist) != 0)
			ActiveFeatures.Add(TEXT("Wrist"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Forearm) != 0)
			ActiveFeatures.Add(TEXT("Forearm"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Elbow) != 0)
			ActiveFeatures.Add(TEXT("Elbow"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Shoulder) != 0)
			ActiveFeatures.Add(TEXT("Shoulder"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Torso) != 0)
			ActiveFeatures.Add(TEXT("Torso"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Ankle) != 0)
			ActiveFeatures.Add(TEXT("Ankle"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Knee) != 0)
			ActiveFeatures.Add(TEXT("Knee"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Hip) != 0)
			ActiveFeatures.Add(TEXT("Hip"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Feet) != 0)
			ActiveFeatures.Add(TEXT("Feet"));


		if (ActiveFeatures.Num() > 0)
		{
			FlagDescription = FString::Join(ActiveFeatures, TEXT(", "));
		}
		else
		{
			FlagDescription = TEXT("None");
		}
		return FlagDescription;
	}

	void AddFlag(ESkinVisibilityFlag InFlagMask)
	{
		FlagMask |= (1 << static_cast<int32>(InFlagMask));
	}

	void AddFlag(const int32 InFlagMask)
	{
		FlagMask |= InFlagMask;
	}

	void RemoveFlag(ESkinVisibilityFlag InFlagMask)
	{
		FlagMask &= ~(1 << static_cast<int32>(InFlagMask));
	}

	void RemoveFlag(const int32 InFlagMask)
	{
		FlagMask &= ~InFlagMask;
	}

	bool HasFlag(ESkinVisibilityFlag InFlag) const
	{
		return (FlagMask & (1 << static_cast<int32>(InFlag))) != 0;
	}

	bool HasFlag(const int32 InFlagMask) const
	{
		return (FlagMask & InFlagMask) == InFlagMask;
	}

	bool HasAnyOfFlags(const int32 InFlagMask) const
	{
		return (FlagMask & InFlagMask) != 0;
	}

	void SetFlags(const int32 InFlags)
	{
		FlagMask = InFlags;
	}

	void ClearAllFlags()
	{
		FlagMask = 0;
		FlagDescription = {};
	}
	
	FString ToString()
	{
		TArray<FString> ActiveFeatures;
		
		if ((FlagMask & (int32)ESkinVisibilityFlag::Wrist) != 0)
			ActiveFeatures.Add(TEXT("Wrist"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Forearm) != 0)
			ActiveFeatures.Add(TEXT("Forearm"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Elbow) != 0)
			ActiveFeatures.Add(TEXT("Elbow"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Shoulder) != 0)
			ActiveFeatures.Add(TEXT("Shoulder"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Torso) != 0)
			ActiveFeatures.Add(TEXT("Torso"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Ankle) != 0)
			ActiveFeatures.Add(TEXT("Ankle"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Knee) != 0)
			ActiveFeatures.Add(TEXT("Knee"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Hip) != 0)
			ActiveFeatures.Add(TEXT("Hip"));
		if ((FlagMask & (int32)ESkinVisibilityFlag::Feet) != 0)
			ActiveFeatures.Add(TEXT("Feet"));

		FString Result = FString::Printf(TEXT("Flags (Mask: %d):\n"), FlagMask);
		if (ActiveFeatures.Num() > 0)
		{
			for (int32 i = 0; i < ActiveFeatures.Num(); ++i)
			{
				Result += ActiveFeatures[i];
				if (i < ActiveFeatures.Num() - 1)
				{
					Result += TEXT(", ");
				}

				// Add a newline after every 3 flags (or after the last flag)
				if ((i + 1) % 3 == 0 || i == ActiveFeatures.Num() - 1)
				{
					Result += TEXT("\n");
				}
			}
		}
		else
		{
			Result += GLOBAL_CONSTANTS::NONE_STRING;
		}

		return Result;
	}
};
