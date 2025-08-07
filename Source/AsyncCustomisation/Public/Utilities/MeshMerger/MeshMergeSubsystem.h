#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Utilities/CustomizationSettings.h"
#include "MeshMergeSubsystem.generated.h"

struct FGameplayTag;
class UAsyncSkeletalMeshMerge;

USTRUCT()
struct FSkeletalMeshArrayKey
{
	GENERATED_BODY()
	
	FSkeletalMeshArrayKey(){}

	FSkeletalMeshArrayKey(TArray<USkeletalMesh*>&& InMeshArray)
		: MeshArray(MoveTemp(InMeshArray))
	{}
	
	FSkeletalMeshArrayKey(const FSkeletalMeshArrayKey& Other)
		: MeshArray(Other.MeshArray)
	{}

	FSkeletalMeshArrayKey(FSkeletalMeshArrayKey&& Other) noexcept
		: MeshArray(MoveTemp(Other.MeshArray))
	{}

	FSkeletalMeshArrayKey& operator=(const FSkeletalMeshArrayKey& Other) noexcept
	{
		MeshArray = Other.MeshArray;
		return *this;
	}

	FSkeletalMeshArrayKey& operator=(FSkeletalMeshArrayKey&& Other) noexcept
	{
		MeshArray = MoveTemp(Other.MeshArray);
		return *this;
	}

	bool operator==(const FSkeletalMeshArrayKey& Other) const
	{
		if (MeshArray.Num() != Other.MeshArray.Num())
		{
			return false;
		}

		for (int32 i = 0; i < MeshArray.Num(); ++i)
		{
			if (MeshArray[i] != Other.MeshArray[i])
			{
				return false;
			}
		}

		return true;
	}
	
	UPROPERTY()
	TArray<USkeletalMesh*> MeshArray;
};

USTRUCT()
struct FMeshToMergeData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	UPROPERTY()
	FGameplayTag SlotTag;
};

FORCEINLINE uint32 GetTypeHash(const FSkeletalMeshArrayKey& Key)
{
	uint32 Hash = 0;

	for (USkeletalMesh* Mesh : Key.MeshArray)
	{
		Hash = HashCombine(Hash, GetTypeHash(Mesh));
	}

	return Hash;
}

DECLARE_DELEGATE_OneParam(FOnMeshMergeCompleteDelegate, USkeletalMesh*);

UCLASS()
class ASYNCCUSTOMISATION_API UMeshMergeSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// FTickableGameObject implementation Begin
	virtual UWorld* GetTickableGameObjectWorld() const override final { return GetWorld(); }
	virtual bool IsAllowedToTick() const override final { return !IsTemplate(); };
	virtual bool IsTickable() const override final { return true; };
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMeshMergeSubsystem, STATGROUP_Tickables);  }
	// FTickableGameObject implementation End

	static bool MergeMeshesWithSettings(const UWorld* World, const TArray<FMeshToMergeData>& MeshesToMergeData, TMap<int32, FGameplayTag>* OutMaterialMap, FOnMeshMergeCompleteDelegate&& OnMeshMergeComplete);

private:
	static EMeshMergeMethod GetCurrentMergeMethod();
	
	static bool SyncMerge(const UWorld* World, const TArray<FMeshToMergeData>& MeshesToMergeData, TMap<int32, FGameplayTag>* OutMaterialMap, FOnMeshMergeCompleteDelegate&& OnMeshMergeComplete);
	
	// UPROPERTY()
	// TMap<FSkeletalMeshArrayKey, USkeletalMesh*> CachedMeshes;
};
