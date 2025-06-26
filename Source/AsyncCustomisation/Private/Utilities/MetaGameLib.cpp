#include "Utilities/MetaGameLib.h"

#include "GameplayTagContainer.h"
#include "Components/Core/Somatotypes.h"
#include "Constants/GlobalConstants.h"
#include "UI/VM_Inventory.h"
#include "Utilities/DataTable/DataTableLibraryTypes.h"

/**
 * @brief Generates the function definition for an equipped map converter.
 * @param FuncName The name of the function (must match the one in DECLARE_EQUIPPED_MAP_CONVERTER).
 * @param SlotTagString The string representation of the GameplayTag to search for (e.g., "Slot.UI.Hat").
 */
#define DEFINE_EQUIPPED_MAP_CONVERTER(FuncName, SlotTagString) \
FInventoryEquippedItemData UMetaGameLib::FuncName(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap) \
{ \
return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName(SlotTagString))); \
}
#undef DEFINE_EQUIPPED_MAP_CONVERTER

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

FInventoryEquippedItemData UMetaGameLib::GetItemFromEquippedMapByTagInternal(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap, const FGameplayTag& SlotTag)
{
	const FInventoryEquippedItemData* Item = InMap.Find(SlotTag);
	return Item ? *Item : FInventoryEquippedItemData();
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapHat(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Hat")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapTorso(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Torso")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapWrists(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Wrists")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapLegs(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Legs")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapFeet(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Feet")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapAccessory(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Accessory")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapBack(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Back")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapHead(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
    return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Head")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapSkin(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
    return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Skin")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapBandana(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Bandana")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapScarf(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Scarf")));
}

FInventoryEquippedItemData UMetaGameLib::ConvertEquippedMapCloak(const TMap<FGameplayTag, FInventoryEquippedItemData>& InMap)
{
	return GetItemFromEquippedMapByTagInternal(InMap, FGameplayTag::RequestGameplayTag(FName("Slot.UI.Cloak")));
}
