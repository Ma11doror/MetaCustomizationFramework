// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/MetaGameLib.h"

#include "Components/Core/Somatotypes.h"
#include "Constants/GlobalConstants.h"
#include "Utilities/DataTable/DataTableLibraryTypes.h"

FPrimaryAssetId UMetaGameLib::GetDefaultSkinAssetIdBySomatotype(const ESomatotype InType)
{
	const UDataTable* DataTable = UMetaGameLib::GetDataTableFromLibrary(EDataTableLibraryType::DefaultSomatotypeSkins);
	//TODO:: log
	ensure(DataTable);
	TArray<FSomatotypeDefaultSkinRow*> Rows;
	DataTable->GetAllRows<FSomatotypeDefaultSkinRow>(StringCast<TCHAR>(__FUNCTION__).Get(), Rows);
	auto FoundRow = Algo::FindByPredicate(Rows, [InType](const FSomatotypeDefaultSkinRow* Row) { return Row->Somatotype == InType; });

	if (FoundRow == nullptr)
	{
		return GLOBAL_CONSTANTS::NONE_ASSET_ID;
	}
	const auto ResultAssetId = (*FoundRow)->DefaultSkinAssetId;
	return ResultAssetId;
}

UDataTable* UMetaGameLib::GetDataTableFromLibrary(EDataTableLibraryType InType)
{
	using namespace GLOBAL_CONSTANTS;
	//TODO:: log
	ensure(!DATA_TABLE_LIBRARY.IsNull());

	// Get if loaded
	const UDataTable* DataTableLibrary = Cast<UDataTable>(DATA_TABLE_LIBRARY.ResolveObject());
	// Load if not loaded 
	if (!DataTableLibrary)
	{
		DataTableLibrary = Cast<UDataTable>(DATA_TABLE_LIBRARY.TryLoad());
	}

	//TODO:: log

	// Find proper datatable
	UDataTable* Result = nullptr;
	DataTableLibrary->ForeachRow<FDataTableLibraryRow>(*FString(__FUNCTION__), [&](const FName& Key, const FDataTableLibraryRow& Row) {
		if (Row.Type == InType)
		{
			Result = Row.Datatable;
		}
	});
	return Result;
}
