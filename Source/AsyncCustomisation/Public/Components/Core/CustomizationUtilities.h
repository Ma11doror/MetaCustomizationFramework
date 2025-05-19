#pragma once

#include <CoreMinimal.h>

#include "Assets/CustomizationDataAsset.h"
#include "Assets/BodyPartAsset.h"
#include "Assets/MaterialCustomizationDataAsset.h"
#include "Assets/MaterialPackCustomizationDA.h"
#include "AsyncCustomisation/Public/Constants/GlobalConstants.h"
#include "Utilities/CustomizationAssetManager.h"
#include "Utilities/MetaGameLib.h"
#include "Utilities/DataTable/DataTableLibraryTypes.h"


class UCustomizationComponent;

namespace CustomizationUtilities
{
	inline UClass* GetClassForCustomizationAsset(const FName& AssetTypeName)
	{
		TMap<FName, UClass*> TypeNameToClass = { {GLOBAL_CONSTANTS::PrimaryCustomizationAssetType, UCustomizationDataAsset::StaticClass() },
			{GLOBAL_CONSTANTS::PrimaryMaterialCustomizationAssetType, UMaterialCustomizationDataAsset::StaticClass() },
			{GLOBAL_CONSTANTS::PrimaryBodyPartAssetType, UBodyPartAsset::StaticClass() },
			{GLOBAL_CONSTANTS::PrimaryMaterialPackCustomizationAssetType, UMaterialPackCustomizationDA::StaticClass() } };

		UClass** Class = TypeNameToClass.Find(AssetTypeName);
		return Class ? *Class : nullptr;
	}
	
	inline UClass* GetClassForCustomizationAsset(const FPrimaryAssetId& InAssetId)
	{
		const auto& PartAssetTypeName = InAssetId.PrimaryAssetType.GetName();
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
	
	inline void SetSkeletalMeshAssetWithMaterials(USkeletalMeshComponent* SkeletalMeshComponent, USkeletalMesh* SourceSkeletalMesh)
	{
		SkeletalMeshComponent->SetSkeletalMeshAsset(SourceSkeletalMesh);
		SetMaterialsFromMesh(SkeletalMeshComponent, SourceSkeletalMesh);
	}

	static void SetBodyPartSkeletalMesh(UCustomizationComponent* Self,
	                                    USkeletalMesh* SourceSkeletalMesh,
	                                    const EBodyPartType TargetBodyPartType);
	
	static void SetSkeletalMesh(UCustomizationComponent* Self,
								USkeletalMesh* SourceSkeletalMesh,
								USkeletalMeshComponent* TargetSkeletalMeshComponent);
}
