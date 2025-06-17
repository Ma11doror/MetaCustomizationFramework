#include "Utilities/MetaGameLib.h"

#include "Components/Core/Somatotypes.h"
#include "Constants/GlobalConstants.h"
#include "UI/VM_Inventory.h"
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

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapToSlot(const TMap<EItemSlot, FInventoryEquippedItemData>& InMap, const EItemSlot& InType)
{
	auto Item = InMap.Find(InType);
	return Item ? *Item : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapHatSlotItem(const TMap<EItemSlot, FInventoryEquippedItemData>& InMap)
{
	auto Item = InMap.Find(EItemSlot::Hat);
	return Item ? *Item : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapBodySlotItem(const TMap<EItemSlot, FInventoryEquippedItemData>& InMap)
{
	auto Item = InMap.Find(EItemSlot::Body);
	return Item ? *Item : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapGlovesSlotItem(const TMap<EItemSlot, FInventoryEquippedItemData>& InMap)
{
	auto Item = InMap.Find(EItemSlot::Wrists);
	return Item ? *Item : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedPantsPantsSlotItem(const TMap<EItemSlot, FInventoryEquippedItemData>& InMap)
{
	auto Item = InMap.Find(EItemSlot::Legs);
	return Item ? *Item : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapFeetSlotItem(const TMap<EItemSlot, FInventoryEquippedItemData>& InMap)
{
	auto Item = InMap.Find(EItemSlot::Feet);
	return Item ? *Item : FInventoryEquippedItemData();
}
