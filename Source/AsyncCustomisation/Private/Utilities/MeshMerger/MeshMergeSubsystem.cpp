#include "Utilities/MeshMerger/MeshMergeSubsystem.h"

#include "SkeletalMeshMerge.h"
#include "Algo/AllOf.h"
#include "Utilities/CustomizationSettings.h"


void UMeshMergeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
}

void UMeshMergeSubsystem::Tick(float DeltaTime)
{

}

bool UMeshMergeSubsystem::MergeMeshesWithSettings(const UWorld* World, const TArray<FMeshToMergeData>& MeshesToMergeData, TMap<int32, FGameplayTag>* OutMaterialMap, FOnMeshMergeCompleteDelegate&& OnMeshMergeComplete)
{
	if (MeshesToMergeData.Num() == 0)
	{
		OnMeshMergeComplete.ExecuteIfBound(nullptr);
		return false;
	}
	
	switch (GetCurrentMergeMethod())
	{
	case EMeshMergeMethod::SyncMeshMerge:
		return SyncMerge(World, MeshesToMergeData, OutMaterialMap, MoveTemp(OnMeshMergeComplete));
	// case EMeshMergeMethod::AsyncMeshMerge:
	// 	return false; //AsyncMergeMeshes(World, MeshesToMergeData, OutMaterialMap, MoveTemp(OnMeshMergeComplete));
	default:
		UE_LOG(LogTemp, Warning, TEXT("UMeshMergeSubsystem::MergeMeshesWithSettings: Unknown merge method, falling back to synchronous"));
		return SyncMerge(World, MeshesToMergeData, OutMaterialMap, MoveTemp(OnMeshMergeComplete));
	}
}

EMeshMergeMethod UMeshMergeSubsystem::GetCurrentMergeMethod()
{
	const UCustomizationSettings* Settings = UCustomizationSettings::Get();
	return Settings ? Settings->GetMeshMergeMethod() : EMeshMergeMethod::SyncMeshMerge;
}

bool UMeshMergeSubsystem::SyncMerge(const UWorld* World, const TArray<FMeshToMergeData>& MeshesToMergeData, TMap<int32, FGameplayTag>* OutMaterialMap, FOnMeshMergeCompleteDelegate&& OnMeshMergeComplete)
{
	if (MeshesToMergeData.IsEmpty())
	{
		OnMeshMergeComplete.ExecuteIfBound(nullptr);
		return false;
	}
    
	USkeletalMesh* BaseMesh = MeshesToMergeData[0].SkeletalMesh;
	if (!ensureMsgf(BaseMesh, TEXT("UMeshMergeSubsystem::ExecuteSynchronousMerge - Base mesh is invalid")))
	{
		OnMeshMergeComplete.ExecuteIfBound(nullptr);
		return false;
	}
    
	TArray<USkeletalMesh*> MeshesToMerge;
	for(const auto& Data : MeshesToMergeData)
	{
		if (Data.SkeletalMesh)
		{
			MeshesToMerge.Add(Data.SkeletalMesh);
		}
	}
	
	if (!ensureMsgf(Algo::AllOf(MeshesToMerge, [](USkeletalMesh* Mesh){ return IsValid(Mesh); }), TEXT("UMeshMergeSubsystem::ExecuteSynchronousMerge - Some of attachments are invalid")))
	{
		OnMeshMergeComplete.ExecuteIfBound(nullptr);
		return false;
	}
	
	USkeletalMesh* MergedMesh = NewObject<USkeletalMesh>(GetTransientPackage());
	if (BaseMesh->GetSkeleton())
	{
		MergedMesh->SetSkeleton(BaseMesh->GetSkeleton());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UMeshMergeSubsystem::ExecuteSynchronousMerge - BaseMesh has no skeleton."));
		OnMeshMergeComplete.ExecuteIfBound(nullptr);
		return false;
	}
	
	// Build the Material Index to Slot Tag map if the output map is provided
	if (OutMaterialMap)
	{
		OutMaterialMap->Empty();
		int32 CurrentMaterialIndex = 0;
		for (const auto& Data : MeshesToMergeData)
		{
			if (Data.SkeletalMesh)
			{
				for (int32 i = 0; i < Data.SkeletalMesh->GetMaterials().Num(); ++i)
				{
					OutMaterialMap->Add(CurrentMaterialIndex, Data.SlotTag);
					CurrentMaterialIndex++;
				}
			}
		}
	}
	
	TArray<FSkelMeshMergeSectionMapping> SectionMappings;
	const int32 StripTopLODs = 0;
    
	FSkeletalMeshMerge Merger(MergedMesh, MeshesToMerge, SectionMappings, StripTopLODs, EMeshBufferAccess::Default);
 
	if (!Merger.DoMerge())
	{
		UE_LOG(LogTemp, Error, TEXT("UMeshMergeSubsystem::ExecuteSynchronousMerge - FSkeletalMeshMerge::DoMerge failed."));
		OnMeshMergeComplete.ExecuteIfBound(nullptr);
		return false;
	}
	
	OnMeshMergeComplete.ExecuteIfBound(MergedMesh);
	return true;
}