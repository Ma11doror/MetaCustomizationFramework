#include "Components/Core/MetaConfig.h"

#include "Constants/GlobalConstants.h"
#include "Meta/MetaManagerSubsystem.h"
#include "Misc/RuntimeErrors.h"


UMetaConfig* UMetaConfig::Get(const UObject* InWorldContextObject, const bool IsRequired)
{
	check(InWorldContextObject)

	const auto* MetaManager = UMetaManagerSubsystem::Get(InWorldContextObject, IsRequired);
	ensure(MetaManager);

	auto* Manager = MetaManager->MetaConfigManager;
	if (IsRequired)
	{
		check(Manager);
	}

	return Manager;
}

const UMetaConfigItemData* UMetaConfig::GetItemConfigBySlug(const UObject* WCO, const FName& InSlug)
{

	ensure(WCO);
	auto* Config = Get(WCO);
	ensure(Config);
	FPrimaryAssetId AssetId = {};
	AssetId.PrimaryAssetName = InSlug;
	AssetId.PrimaryAssetType = GLOBAL_CONSTANTS::PrimaryItemAssetType;
	return Config->GetItemConfigByPrimaryAssetId(AssetId);;
}

const UMetaConfigItemData* UMetaConfig::GetItemConfigByPrimaryAssetId(const FPrimaryAssetId& InAssetId)
{
	const auto* ItemConfig = AllItemsConfig.Find(InAssetId.PrimaryAssetName);
	if (!ensureAsRuntimeWarning(ItemConfig))
	{
		return nullptr;
	}
	return *ItemConfig;
}

void UMetaConfig::Init()
{
	
}
