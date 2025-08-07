#pragma once

#include <CoreMinimal.h>

#include "Assets/CustomizationDataAsset.h"
#include "Assets/BodyPartAsset.h"
#include "Assets/MaterialCustomizationDataAsset.h"
#include "Assets/MaterialPackCustomizationDA.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "Utilities/MetaGameLib.h"
#include "Utilities/DataTable/DataTableLibraryTypes.h"


struct FMergeResult;
class UCustomizationComponent;

namespace CustomizationUtilities
{
	inline UClass* GetClassForCustomizationAsset(const FName& AssetTypeName)
	{
		TMap<FName, UClass*> TypeNameToClass = {
			{GLOBAL_CONSTANTS::PrimaryCustomizationAssetType, UCustomizationDataAsset::StaticClass() },
			{GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType, UMaterialCustomizationDataAsset::StaticClass() },
			{GLOBAL_CONSTANTS::PrimaryBodyPartAssetType, UBodyPartAsset::StaticClass() },
			{GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType, UMaterialPackCustomizationDA::StaticClass() }
		};

		UClass** Class = TypeNameToClass.Find(AssetTypeName);
		return Class ? *Class : nullptr;
	}
	
	inline UClass* GetClassForCustomizationAsset(const FPrimaryAssetId& InAssetId)
	{
		const auto& PartAssetTypeName = InAssetId.PrimaryAssetType.GetName();
		//UE_LOG(LogTemp, Display, TEXT("GetClassForCustomizationAsset:: %s"), *PartAssetTypeName.ToString());
		auto* Result = GetClassForCustomizationAsset(PartAssetTypeName);
		return Result;
	}

	inline void SetMaterialOnMesh(
		const UMaterialCustomizationDataAsset* InMaterialCustomizationAsset, UPrimitiveComponent* CustomizableMesh)
	{
		ensure(InMaterialCustomizationAsset); //, TEXT("Invalid MaterialCustomizationAsset!")
		ensure(CustomizableMesh); //, TEXT("Invalid CustomizableMesh!")

		for (const auto& [Index, Material] : InMaterialCustomizationAsset->IndexWithApplyingMaterial)
		{
			CustomizableMesh->SetMaterial(Index, Material);
		}
	}

	inline FPrimaryAssetId GetSomatotypeAssetId(const ESomatotype InType)
	{
		const UDataTable* DataTable = UMetaGameLib::GetDataTableFromLibrary(EDataTableLibraryType::Somatotypes);
		ensure(DataTable);
		TArray<FSomatotypeAssetRow*> Rows;
		DataTable->GetAllRows<FSomatotypeAssetRow>(StringCast<TCHAR>(__FUNCTION__).Get(), Rows);
		auto FoundRow = Algo::FindByPredicate(Rows, [InType](const FSomatotypeAssetRow* Row)
		{
			return Row->Somatotype == InType;
		});
		if(!ensure(FoundRow))
		{
			return FPrimaryAssetId{};
		}
		ensure(FoundRow);
		return (*FoundRow)->SomatotypeAssetId;
	}

	inline void SetMaterialsFromMesh(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SourceSkeletalMesh)
	{
		if (!IsValid(SourceSkeletalMesh) || !IsValid(SkeletalMeshComponent))
		{
			return;
		}
		const auto& SourceMaterials = SourceSkeletalMesh->GetMaterials();
		for (int32 Index = 0; Index < SourceMaterials.Num(); ++Index)
		{
			const auto& SourceMaterial = SourceMaterials[Index].MaterialInterface;
			SkeletalMeshComponent->SetMaterial(Index, SourceMaterial);
		}
	}
	
	inline void ApplyDefaultMaterials(USkeletalMeshComponent* SkeletalMeshComponent, const FBodyPartVariant* Variant)
	{
		if (!SkeletalMeshComponent) return;

		if (Variant && !Variant->DefaultMaterials.IsEmpty())
		{
			for (int32 Index = 0; Index < Variant->DefaultMaterials.Num(); ++Index)
			{
				SkeletalMeshComponent->SetMaterial(Index, Variant->DefaultMaterials[Index]);
			}
		}
		else if (USkeletalMesh* SourceSkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			SetMaterialsFromMesh(SkeletalMeshComponent, SourceSkeletalMesh);
		}
	}
	
	inline void SetSkeletalMeshAssetWithMaterials(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SourceSkeletalMesh, const FBodyPartVariant* Variant)
	{
		SkeletalMeshComponent->SetSkeletalMeshAsset(SourceSkeletalMesh);
		ApplyDefaultMaterials(SkeletalMeshComponent, Variant);
	}

	static void SetBodyPartSkeletalMesh(UCustomizationComponent* Self,
	                                    USkeletalMesh* SourceSkeletalMesh,
	                                    const FBodyPartVariant* Variant,
	                                    const FGameplayTag& TargetSlotTag);
	
	static void SetSkeletalMesh(UCustomizationComponent* Self,
								USkeletalMesh* SourceSkeletalMesh,
								USkeletalMeshComponent* TargetSkeletalMeshComponent,
								const FBodyPartVariant* Variant,
								const FGameplayTag& TargetSlotTag);
}
