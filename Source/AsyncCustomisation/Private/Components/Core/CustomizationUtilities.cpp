#include "AsyncCustomisation/Public/Components/Core/CustomizationUtilities.h"

#include "BaseCharacter.h"
#include "Utilities/CommonUtilities.h"
#include "AsyncCustomisation/Public/Components/CustomizationComponent.h"

void CustomizationUtilities::SetBodyPartSkeletalMesh(
	UCustomizationComponent* Self, USkeletalMesh* SourceSkeletalMesh, const EBodyPartType TargetBodyPartType)
{
	if (!Self) // Always check for Self validity first
	{
		UE_LOG(LogTemp, Error, TEXT("SetBodyPartSkeletalMesh: UCustomizationComponent is null."));
		return;
	}

	const auto& Skeletals = Self->GetSkeletals();
	USkeletalMeshComponent* const* TargetBodyPartSkeletalPtr = Skeletals.Find(TargetBodyPartType);

	if (!TargetBodyPartSkeletalPtr || !(*TargetBodyPartSkeletalPtr))
	{
		UE_LOG(LogTemp, Error, TEXT("SetBodyPartSkeletalMesh: Target SkeletalMeshComponent not found or invalid for BodyPartType: %s"), *UEnum::GetValueAsString(TargetBodyPartType));
		return;
	}

	USkeletalMeshComponent* TargetBodyPartSkeletal = *TargetBodyPartSkeletalPtr;
	CustomizationUtilities::SetSkeletalMesh(Self, SourceSkeletalMesh, TargetBodyPartSkeletal, TargetBodyPartType);
}

void CustomizationUtilities::SetSkeletalMesh(
	UCustomizationComponent* Self, USkeletalMesh* SourceSkeletalMesh, USkeletalMeshComponent* TargetSkeletalMeshComponent, const EBodyPartType TargetBodyPartType)
{
	if (!Self)
	{
		return;
	}
	if (!TargetSkeletalMeshComponent)
	{
		return;
	}

	const USkeletalMesh* CurrentTargetMesh = TargetSkeletalMeshComponent->GetSkeletalMeshAsset();

	if (CurrentTargetMesh != SourceSkeletalMesh)
	{
		UE_LOG(LogTemp, Display, TEXT("SetSkeletalMesh: Changing mesh for %s from %s to %s"),
		       *TargetSkeletalMeshComponent->GetName(),
		       CurrentTargetMesh ? *CurrentTargetMesh->GetName() : TEXT("nullptr"),
		       SourceSkeletalMesh ? *SourceSkeletalMesh->GetName() : TEXT("nullptr")
		);

		if (TargetBodyPartType == EBodyPartType::BodySkin)
		{
			TargetSkeletalMeshComponent->SetSkeletalMeshAsset(SourceSkeletalMesh); // Set mesh
			if (SourceSkeletalMesh)
			{
				Self->ApplyCachedMaterialToBodySkinMesh();
			}
		}
		else 
		{
			SetSkeletalMeshAssetWithMaterials(TargetSkeletalMeshComponent, SourceSkeletalMesh);
		}
		
		if (SourceSkeletalMesh)
		{
			ABaseCharacter* OwningCharacter = Self->GetOwningCharacter();
			if (OwningCharacter && OwningCharacter->AnimBP)
			{
				TargetSkeletalMeshComponent->SetAnimInstanceClass(OwningCharacter->AnimBP);
				OwningCharacter->SetupMasterPose();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SetSkeletalMesh: OwningCharacter or AnimBP is null for %s. Animation not set."), *TargetSkeletalMeshComponent->GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("SetSkeletalMesh: Cleared mesh for %s"), *TargetSkeletalMeshComponent->GetName());
			// TargetSkeletalMeshComponent->SetMasterPoseComponent(nullptr);
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("SetSkeletalMesh: Mesh for %s is already %s. No change."),
		       *TargetSkeletalMeshComponent->GetName(),
		       SourceSkeletalMesh ? *SourceSkeletalMesh->GetName() : TEXT("nullptr")
		);
		if (TargetBodyPartType == EBodyPartType::BodySkin && SourceSkeletalMesh)
		{
			Self->ApplyCachedMaterialToBodySkinMesh();
		}
	}
}
