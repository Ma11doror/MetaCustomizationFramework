// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "DataTableLibraryTypes.generated.h"


UENUM(BlueprintType)
enum class EDataTableLibraryType : uint8
{
	None = 0,
	Somatotypes = 1,
	DefaultSomatotypeSkins = 2,
};
ENUM_RANGE_BY_FIRST_AND_LAST(EDataTableLibraryType, EDataTableLibraryType::None, EDataTableLibraryType::DefaultSomatotypeSkins)

USTRUCT(BlueprintType)
struct FSomatotypeDefaultSkinRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESomatotype Somatotype = ESomatotype::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPrimaryAssetId DefaultSkinAssetId;
};

USTRUCT(BlueprintType)
struct FDataTableLibraryRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	EDataTableLibraryType Type = EDataTableLibraryType::None;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDataTable* Datatable = nullptr;
};

USTRUCT(BlueprintType)
struct FSomatotypeAssetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESomatotype Somatotype = ESomatotype::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPrimaryAssetId SomatotypeAssetId;
};