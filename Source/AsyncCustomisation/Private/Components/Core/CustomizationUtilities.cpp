#include "AsyncCustomisation/Public/Components/Core/CustomizationUtilities.h"

#include "BaseCharacter.h"
#include "Utilities/CommonUtilities.h"
#include "AsyncCustomisation/Public/Components/CustomizationComponent.h"

void CustomizationUtilities::SetBodyPartSkeletalMesh(
	UCustomizationComponent* Self, USkeletalMesh* SourceSkeletalMesh, const FBodyPartVariant* Variant, const FGameplayTag& TargetSlotTag)
{
	if (!Self)
	{
		UE_LOG(LogTemp, Error, TEXT("SetBodyPartSkeletalMesh: UCustomizationComponent is null."));
		return;
	}

	if (!TargetSlotTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SetBodyPartSkeletalMesh: Target SlotTag is invalid."));
		return;
	}

	USkeletalMeshComponent* TargetBodyPartSkeletal = Self->CreateOrGetMeshComponentForSlot(TargetSlotTag);

	if (!TargetBodyPartSkeletal)
	{
		return;
	}
	
	SetSkeletalMesh(Self, SourceSkeletalMesh, TargetBodyPartSkeletal, Variant, TargetSlotTag);
}

void CustomizationUtilities::SetSkeletalMesh(
	UCustomizationComponent* Self, USkeletalMesh* SourceSkeletalMesh, USkeletalMeshComponent* TargetSkeletalMeshComponent, const FBodyPartVariant* Variant, const FGameplayTag& TargetSlotTag)
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
	const FGameplayTag BodySkinSlotTag = FGameplayTag::RequestGameplayTag(GLOBAL_CONSTANTS::BodySkinSlotTagName);

	if (CurrentTargetMesh != SourceSkeletalMesh)
	{
		UE_LOG(LogTemp, Display, TEXT("SetSkeletalMesh: Changing mesh for %s from %s to %s"),
			   *TargetSkeletalMeshComponent->GetName(),
			   CurrentTargetMesh ? *CurrentTargetMesh->GetName() : TEXT("nullptr"),
			   SourceSkeletalMesh ? *SourceSkeletalMesh->GetName() : TEXT("nullptr")
		);

		if (TargetSlotTag == BodySkinSlotTag)
		{
			TargetSkeletalMeshComponent->SetSkeletalMeshAsset(SourceSkeletalMesh);
			if (SourceSkeletalMesh)
			{
				Self->ApplyCachedMaterialToBodySkinMesh();
			}
		}
		else 
		{
			SetSkeletalMeshAssetWithMaterials(TargetSkeletalMeshComponent, SourceSkeletalMesh, Variant);
		}
		
		if (!SourceSkeletalMesh)
		{
			UE_LOG(LogTemp, Display, TEXT("SetSkeletalMesh: Cleared mesh for %s"), *TargetSkeletalMeshComponent->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("SetSkeletalMesh: Mesh for %s is already %s. No change."),
			   *TargetSkeletalMeshComponent->GetName(),
			   SourceSkeletalMesh ? *SourceSkeletalMesh->GetName() : TEXT("nullptr")
		);
		if (TargetSlotTag == BodySkinSlotTag && SourceSkeletalMesh)
		{
			Self->ApplyCachedMaterialToBodySkinMesh();
		}
	}
}