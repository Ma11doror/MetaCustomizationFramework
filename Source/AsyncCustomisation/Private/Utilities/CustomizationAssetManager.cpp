#include "AsyncCustomisation/Public/Utilities/CustomizationAssetManager.h"

#include "Components/Core/Assets/BodyPartAsset.h"
#include "Components/Core/Assets/CustomizationDataAsset.h"
#include "Components/Core/Assets/MaterialCustomizationDataAsset.h"
#include "Components/Core/Assets/MaterialPackCustomizationDA.h"
#include "Utilities/CommonUtilities.h"

UCustomizationAssetManager* UCustomizationAssetManager::GetCustomizationAssetManager()
{
	UCustomizationAssetManager* AssetManager = Cast<UCustomizationAssetManager>(GEngine->AssetManager);
	checkf(AssetManager, TEXT("Invalid AssetManager class in DefaultEngine.ini"));
	return AssetManager;
}

UBodyPartAsset* UCustomizationAssetManager::LoadBodyPartAssetSync(FPrimaryAssetId InBodyPartId)
{
	FPrimaryAssetTypeInfo TypeInfo;
	if (GetPrimaryAssetTypeInfo(InBodyPartId.PrimaryAssetType, TypeInfo) && !TypeInfo.bHasBlueprintClasses)
	{
		const TSoftObjectPtr<UBodyPartAsset> SoftBodyPart = TSoftObjectPtr<UBodyPartAsset>(GetPrimaryAssetPath(InBodyPartId));
		return SoftBodyPart.LoadSynchronous();
	}
	return nullptr;
}

void UCustomizationAssetManager::LoadBodyPartAssetAsync(FPrimaryAssetId InBodyPartId, FOnBodyPartLoaded OnLoadDelegate)
{
	if (!InBodyPartId.IsValid())
	{
		return;
	}

	// Checking if asset already loaded
	UObject* Asset = GetPrimaryAssetObject(InBodyPartId);
	if (Asset != nullptr)
	{
		UBodyPartAsset* BodyPartAsset = Cast<UBodyPartAsset>(Asset);
		OnLoadDelegate.ExecuteIfBound(BodyPartAsset);
	}
	else
	{
		// TODO: Implement bundles support
		const TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(InBodyPartId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			if (!LoadHandle->HasLoadCompleted())
			{
				LoadHandle->BindCompleteDelegate(
					FStreamableDelegate::CreateUObject(this, &UCustomizationAssetManager::OnBodyPartAssetLoaded, LoadHandle, OnLoadDelegate));
			}
		}
	}
}

void UCustomizationAssetManager::LoadBodyPartAssetListAsync(TArray<FPrimaryAssetId> InBodyPartIds,
	FOnBodyPartListLoaded OnLoadDelegate)
{
	const TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAssets(InBodyPartIds, TArray<FName>());
	if (LoadHandle.IsValid())
	{
		if (!LoadHandle->HasLoadCompleted())
		{
			LoadHandle->BindCompleteDelegate(
				FStreamableDelegate::CreateUObject(this, &UCustomizationAssetManager::OnBodyPartAssetListLoaded, LoadHandle, OnLoadDelegate));
			return;
		}
	}

	TArray<UBodyPartAsset*> Assets;
	Assets.Reserve(InBodyPartIds.Num());
	for (const auto& BodyPartId : InBodyPartIds)
	{
		UObject* Asset = GetPrimaryAssetObject(BodyPartId);
		if (Asset != nullptr)
		{
			UBodyPartAsset* BodyPartAsset = Cast<UBodyPartAsset>(Asset);
			ensure(BodyPartAsset);
			Assets.Add(BodyPartAsset);
		}
	}
	OnLoadDelegate.ExecuteIfBound(Assets);
}

void UCustomizationAssetManager::LoadCustomizationAssetAsync(FPrimaryAssetId InCustomizationId,
	FOnCustomizationLoaded OnLoadDelegate)
{
	if (!InCustomizationId.IsValid())
	{
		return;
	}

	// Checking if asset already loaded
	UObject* Asset = GetPrimaryAssetObject(InCustomizationId);
	if (Asset != nullptr)
	{
		UCustomizationDataAsset* CustomizationAsset = Cast<UCustomizationDataAsset>(Asset);
		OnLoadDelegate.ExecuteIfBound(CustomizationAsset);
	}
	else
	{
		// TODO: Implement bundles support
		const TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(InCustomizationId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			if (!LoadHandle->HasLoadCompleted())
			{
				LoadHandle->BindCompleteDelegate(
					FStreamableDelegate::CreateUObject(this, &UCustomizationAssetManager::OnCustomizationAssetLoaded, LoadHandle, OnLoadDelegate));
			}
		}
	}
}

UCustomizationDataAsset* UCustomizationAssetManager::LoadCustomizationAssetSync(FPrimaryAssetId InCustomizationId)
{
	if (!InCustomizationId.IsValid())
	{
		return nullptr;
	}

	// Checking if asset already loaded
	UObject* Asset = GetPrimaryAssetObject(InCustomizationId);
	if (Asset != nullptr)
	{
		UCustomizationDataAsset* CustomizationAsset = Cast<UCustomizationDataAsset>(Asset);
		return CustomizationAsset;
	}

	FPrimaryAssetTypeInfo TypeInfo;
	if (GetPrimaryAssetTypeInfo(InCustomizationId.PrimaryAssetType, TypeInfo) && !TypeInfo.bHasBlueprintClasses)
	{
		const TSoftObjectPtr<UCustomizationDataAsset> SoftCustomization =
			TSoftObjectPtr<UCustomizationDataAsset>(GetPrimaryAssetPath(InCustomizationId));
		return SoftCustomization.LoadSynchronous();
	}
	return nullptr;
}

void UCustomizationAssetManager::LoadMaterialCustomizationAssetAsync(FPrimaryAssetId InCustomizationId,
	FOnMaterialCustomizationLoaded OnLoadDelegate)
{
	if (!InCustomizationId.IsValid())
	{
		return;
	}

	// Checking if asset already loaded
	UObject* Asset = GetPrimaryAssetObject(InCustomizationId);
	if (Asset != nullptr)
	{
		UMaterialCustomizationDataAsset* CustomizationAsset = Cast<UMaterialCustomizationDataAsset>(Asset);
		OnLoadDelegate.ExecuteIfBound(CustomizationAsset);
	}
	else
	{
		// TODO: Implement bundles support
		const TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(InCustomizationId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			if (!LoadHandle->HasLoadCompleted())
			{
				LoadHandle->BindCompleteDelegate(
					FStreamableDelegate::CreateUObject(this, &UCustomizationAssetManager::OnMaterialCustomizationLoaded, LoadHandle, OnLoadDelegate));
			}
		}
	}
}

void UCustomizationAssetManager::LoadMaterialPackCustomizationAssetAsync(FPrimaryAssetId InCustomizationId,
	FOnMaterialPackLoaded OnLoadDelegate)
{
	if (!InCustomizationId.IsValid())
	{
		return;
	}

	// Checking if asset already loaded
	UObject* Asset = GetPrimaryAssetObject(InCustomizationId);
	if (Asset != nullptr)
	{
		UMaterialPackCustomizationDA* CustomizationAsset = Cast<UMaterialPackCustomizationDA>(Asset);
		OnLoadDelegate.ExecuteIfBound(CustomizationAsset);
	}
	else
	{
		// TODO: Implement bundles support
		const TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAsset(InCustomizationId, TArray<FName>());
		if (LoadHandle.IsValid())
		{
			if (!LoadHandle->HasLoadCompleted())
			{
				LoadHandle->BindCompleteDelegate(FStreamableDelegate::CreateUObject(
					this, &UCustomizationAssetManager::OnMaterialPackCustomizationAssetLoaded, LoadHandle, OnLoadDelegate));
			}
		}
	}
}

UItemMetaAsset* UCustomizationAssetManager::LoadItemMetaAssetSync(FName InItemSlug)
{
	auto* AssetManager = GetCustomizationAssetManager();
	ensure(AssetManager); // TEXT("Invalid AssetManager!"), nullptr
	const FPrimaryAssetId AssetId = CommonUtilities::ItemSlugToAssetId(InItemSlug);
	return AssetManager->LoadItemMetaAssetSync(AssetId);
}

UItemMetaAsset* UCustomizationAssetManager::LoadItemMetaAssetSync(FPrimaryAssetId InItemAssetId)
{
	FPrimaryAssetTypeInfo TypeInfo;
	if (GetPrimaryAssetTypeInfo(InItemAssetId.PrimaryAssetType, TypeInfo) && !TypeInfo.bHasBlueprintClasses)
	{
		const TSoftObjectPtr<UItemMetaAsset> SoftBodyPart = TSoftObjectPtr<UItemMetaAsset>(GetPrimaryAssetPath(InItemAssetId));
		return SoftBodyPart.LoadSynchronous();
	}
	return nullptr;
}

TArray<FPrimaryAssetType> UCustomizationAssetManager::GetPrimaryAssetTypes(const TArray<FPrimaryAssetType>& ExcludeList)
{
	TArray<FPrimaryAssetTypeInfo> AssetTypeInfoList;
	GetPrimaryAssetTypeInfoList(AssetTypeInfoList);

	TArray<FPrimaryAssetType> Result;
	Algo::Transform(AssetTypeInfoList, Result,
		[](const FPrimaryAssetTypeInfo& AssetTypeInfo) { return FPrimaryAssetType(AssetTypeInfo.PrimaryAssetType); });

	Result.RemoveAll([&](const FPrimaryAssetType& InType) { return ExcludeList.Contains(InType); });

	return Result;
}

TArray<FPrimaryAssetId> UCustomizationAssetManager::GetAllPrimaryAssetIds()
{
	TArray<FPrimaryAssetId> Result;

	TArray<FPrimaryAssetType> Exclude;
	TArray<FPrimaryAssetType> AllPrimaryAssetsTypes = GetPrimaryAssetTypes(Exclude);

	for (const auto& Type : AllPrimaryAssetsTypes)
	{
		TArray<FPrimaryAssetId> Temp;
		GetPrimaryAssetIdList(Type, Temp);
		Result.Append(Temp);
	}

	return Result;
}

FPrimaryAssetId UCustomizationAssetManager::GetPrimaryAssetIdByName(FName InName)
{
	const TArray<FPrimaryAssetId> AllPrimaryAssetIds = GetAllPrimaryAssetIds();
	auto* ResultPtr =
		AllPrimaryAssetIds.FindByPredicate([InName](const FPrimaryAssetId& InAssetId) { return InAssetId.PrimaryAssetName == InName; });
	return ResultPtr ? *ResultPtr : NONE_ASSET_ID;
}

void UCustomizationAssetManager::OnBodyPartAssetLoaded(TSharedPtr<FStreamableHandle> LoadHandle,
	FOnBodyPartLoaded DelegateToCall) const
{
	if (LoadHandle.IsValid())
	{
		UBodyPartAsset* BodyPartAsset = Cast<UBodyPartAsset>(LoadHandle->GetLoadedAsset());
		DelegateToCall.ExecuteIfBound(BodyPartAsset);
		LoadHandle.Reset();
	}
}

void UCustomizationAssetManager::OnBodyPartAssetListLoaded(TSharedPtr<FStreamableHandle> LoadHandle,
	FOnBodyPartListLoaded DelegateToCall) const
{
	TArray<UBodyPartAsset*> BodyPartAssets;
	if (LoadHandle.IsValid())
	{
		TArray<UObject*> LoadedAssets;
		LoadHandle->GetLoadedAssets(LoadedAssets);
		for (const auto& LoadedAsset : LoadedAssets)
		{
			BodyPartAssets.Add(Cast<UBodyPartAsset>(LoadedAsset));
		}
	}
	DelegateToCall.ExecuteIfBound(BodyPartAssets);
	LoadHandle.Reset();
}

void UCustomizationAssetManager::OnMaterialCustomizationLoaded(TSharedPtr<FStreamableHandle> LoadHandle,
	FOnMaterialCustomizationLoaded DelegateToCall) const
{
	if (LoadHandle.IsValid())
	{
		UMaterialCustomizationDataAsset* CustomizationAsset = Cast<UMaterialCustomizationDataAsset>(LoadHandle->GetLoadedAsset());
		DelegateToCall.ExecuteIfBound(CustomizationAsset);
		LoadHandle.Reset();
	}
}

void UCustomizationAssetManager::OnCustomizationAssetLoaded(TSharedPtr<FStreamableHandle> LoadHandle,
	FOnCustomizationLoaded DelegateToCall) const
{
	if (LoadHandle.IsValid())
	{
		UCustomizationDataAsset* CustomizationAsset = Cast<UCustomizationDataAsset>(LoadHandle->GetLoadedAsset());
		DelegateToCall.ExecuteIfBound(CustomizationAsset);
		LoadHandle.Reset();
	}
}

void UCustomizationAssetManager::OnMaterialPackCustomizationAssetLoaded(TSharedPtr<FStreamableHandle> LoadHandle,
	FOnMaterialPackLoaded DelegateToCall) const
{
	if (LoadHandle.IsValid())
	{
		UMaterialPackCustomizationDA* CustomizationAsset = Cast<UMaterialPackCustomizationDA>(LoadHandle->GetLoadedAsset());
		DelegateToCall.ExecuteIfBound(CustomizationAsset);
		LoadHandle.Reset();
	}
}
