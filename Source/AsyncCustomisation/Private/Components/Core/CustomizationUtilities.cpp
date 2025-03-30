#include "AsyncCustomisation/Public/Components/Core/CustomizationUtilities.h"

#include "BaseCharacter.h"
#include "Utilities/CommonUtilities.h"
#include "AsyncCustomisation/Public/Components/CustomizationComponent.h"

void CustomizationUtilities::SetBodyPartSkeletalMesh(
	UCustomizationComponent* Self, USkeletalMesh* SourceSkeletalMesh, const EBodyPartType TargetBodyPartType)
{
	USkeletalMeshComponent* const* TargetBodyPartSkeletalPtr = Self->GetSkeletals().Find(TargetBodyPartType);
	ensure(TargetBodyPartSkeletalPtr);
	USkeletalMeshComponent* TargetBodyPartSkeletal = *TargetBodyPartSkeletalPtr;
	SetSkeletalMesh(Self, SourceSkeletalMesh, TargetBodyPartSkeletal);
}

void CustomizationUtilities::SetSkeletalMesh(
	UCustomizationComponent* Self, USkeletalMesh* SourceSkeletalMesh, USkeletalMeshComponent* TargetSkeletalMeshComponent)
{
	const auto TargetSkeletalMesh = TargetSkeletalMeshComponent->GetSkeletalMeshAsset();
	if (TargetSkeletalMesh != SourceSkeletalMesh)
	{
		// TODO:: log
		TargetSkeletalMeshComponent->SetSkeletalMesh(nullptr);
	}
	UE_LOG(LogTemp, Display, TEXT("Utils : SetSkeletalMesh:  %s to target %s "), *SourceSkeletalMesh->GetPathName(), *TargetSkeletalMeshComponent->GetPathName());
	SetSkeletalMeshAssetWithMaterials(TargetSkeletalMeshComponent, SourceSkeletalMesh);

	ABaseCharacter* OwningCharacter = Self->GetOwningCharacter();
	ensure(OwningCharacter);
	
	TargetSkeletalMeshComponent->SetAnimInstanceClass(OwningCharacter->AnimBP);
	OwningCharacter->SetupMasterPose();
}