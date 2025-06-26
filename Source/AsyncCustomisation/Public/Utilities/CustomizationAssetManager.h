#pragma once

#include "CoreMinimal.h"

#include "Engine/AssetManager.h"
#include "Utilities/Cache.h"
#include "CustomizationAssetManager.generated.h"

struct FGameplayTag;
class UVictoryStanceAnimAsset;
class UFatalityAnimAsset;
class UAbilityMetaAsset;
class UBodyPartAsset;
class UItemMetaAsset;
class ULeaguesDataAsset;
class UCustomizationDataAsset;
class UMaterialCustomizationDataAsset;
class UMaterialPackCustomizationDA;

UCLASS(BlueprintType)
class ASYNCCUSTOMISATION_API UCustomizationAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:

	DECLARE_DELEGATE_OneParam(FOnBodyPartLoaded, UBodyPartAsset* /*InBodyPart*/)
	DECLARE_DELEGATE_OneParam(FOnBodyPartListLoaded, TArray<UBodyPartAsset*>& /*InBodyPartList*/);
	DECLARE_DELEGATE_OneParam(FOnMaterialPackLoaded, UMaterialPackCustomizationDA* /*InOnMaterialPack*/)
	DECLARE_DELEGATE_OneParam(FOnCustomizationLoaded, UCustomizationDataAsset* /*InCustomization*/)
	DECLARE_DELEGATE_OneParam(FOnMaterialCustomizationLoaded, UMaterialCustomizationDataAsset* /*InMaterialCustomization*/)

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static UCustomizationAssetManager* GetCustomizationAssetManager();


	virtual UBodyPartAsset* LoadBodyPartAssetSync(FPrimaryAssetId InBodyPartId);
	virtual void LoadBodyPartAssetAsync(FPrimaryAssetId InBodyPartId, FOnBodyPartLoaded OnLoadDelegate);
	virtual void LoadBodyPartAssetListAsync(TArray<FPrimaryAssetId> InBodyPartIds, FOnBodyPartListLoaded OnLoadDelegate);

	virtual void LoadCustomizationAssetAsync(FPrimaryAssetId InCustomizationId, FOnCustomizationLoaded OnLoadDelegate);
	virtual UCustomizationDataAsset* LoadCustomizationAssetSync(FPrimaryAssetId InCustomizationId);
	virtual void LoadMaterialCustomizationAssetAsync(FPrimaryAssetId InCustomizationId, FOnMaterialCustomizationLoaded OnLoadDelegate);
	virtual void LoadMaterialPackCustomizationAssetAsync(FPrimaryAssetId InCustomizationId, FOnMaterialPackLoaded OnLoadDelegate);
	

	static UItemMetaAsset* LoadItemMetaAssetSync(FName InItemSlug);
	virtual UItemMetaAsset* LoadItemMetaAssetSync(FPrimaryAssetId InItemAssetId);

	using Super::LoadPrimaryAsset;

	template <typename T, TEMPLATE_REQUIRES(TIsDerivedFrom<T, UPrimaryDataAsset>::Value)>
	T* LoadPrimaryAsset(const FPrimaryAssetId& InPrimaryAssetId)
	{
		ensure(InPrimaryAssetId.IsValid());
		FPrimaryAssetTypeInfo TypeInfo;
		if (GetPrimaryAssetTypeInfo(InPrimaryAssetId.PrimaryAssetType, TypeInfo))
		{
			const TSoftObjectPtr<T> SoftBodyPart = TSoftObjectPtr<T>(GetPrimaryAssetPath(InPrimaryAssetId));
			return SoftBodyPart.LoadSynchronous();
		}
		return nullptr;
	}

	template <typename TAssetType, TEMPLATE_REQUIRES(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value)>
	static TArray<TAssetType*> GetAssetsListFromAssetIds(TArray<FPrimaryAssetId> AssetIds)
	{
		TArray<TAssetType*> Result;
		if (AssetIds.IsEmpty())
		{
			return Result;
		}

		const UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (!AssetManager)
		{
			return Result;
		}

		for (const FPrimaryAssetId& PrimaryAsset : AssetIds)
		{
			if (!PrimaryAsset.IsValid())
			{
				continue;
			}

			TAssetType* AssetObject = AssetManager->GetPrimaryAssetObject<TAssetType>(PrimaryAsset);
			if (AssetObject != nullptr)
			{
				Result.Add(AssetObject);
			}
		}
		return Result;
	}

	static TArray<UClass*> GetAssetsClassList(TArray<FSoftObjectPath> AssetList)
	{
		TArray<UClass*> Result;
		if (AssetList.IsEmpty())
		{
			return Result;
		}

		for (const FSoftObjectPath& AssetPath : AssetList)
		{
			if (!AssetPath.IsValid())
			{
				continue;
			}

			UClass* LoadedClass = Cast<UClass>(AssetPath.ResolveObject());
			if (LoadedClass != nullptr)
			{
				Result.Add(LoadedClass);
			}
		}
		return Result;
	}

	template <typename TAssetType>
	void AsyncLoadAssetList(const TArray<FPrimaryAssetId>& AssetIds, TFunction<void(TArray<TAssetType*>)>&& Callback)
	{
		static_assert(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value, "TAssetType should be derived from UPrimaryDataAsset.");
		auto CallbackLambda = [this, AssetIds, Callback = MoveTemp(Callback)]() {
			TArray<TAssetType*> Assets = GetAssetsListFromAssetIds<TAssetType>(AssetIds);
			Callback(MoveTemp(Assets));
		};
		AsyncLoadAssetsInternal(AssetIds, MoveTemp(CallbackLambda));
	}

	template <typename TAssetType>
	void SyncLoadAssetList(const TArray<FPrimaryAssetId>& AssetIds, TFunction<void(TArray<TAssetType*>)>&& Callback)
	{
		static_assert(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value, "TAssetType should be derived from UPrimaryDataAsset.");
		auto CallbackLambda = [this, AssetIds, Callback = MoveTemp(Callback)]() {
			TArray<TAssetType*> Assets = GetAssetsListFromAssetIds<TAssetType>(AssetIds);
			Callback(MoveTemp(Assets));
		};
		SyncLoadAssetsInternal(AssetIds, MoveTemp(CallbackLambda));
	}

	template <typename TAssetType>
	static void StaticAsyncLoadAssetList(const TArray<FPrimaryAssetId>& AssetIds, TFunction<void(TArray<TAssetType*>)>&& Callback)
	{
		auto* AssetManager = GetCustomizationAssetManager();
		ensure(AssetManager);
		AssetManager->AsyncLoadAssetList<TAssetType>(AssetIds, MoveTemp(Callback));
	}

	template <typename TAssetType>
	static void StaticSyncLoadAssetList(const TArray<FPrimaryAssetId>& AssetIds, TFunction<void(TArray<TAssetType*>)>&& Callback)
	{
		auto* AssetManager = GetCustomizationAssetManager();
		ensure(AssetManager);
		AssetManager->SyncLoadAssetList<TAssetType>(AssetIds, MoveTemp(Callback));
	}

	template <typename TAssetType, typename TCallerType, typename... VarTypes>
	void AsyncLoadAssetList(const TArray<FPrimaryAssetId>& AssetIds, TCallerType* Caller,
		void (TCallerType::*Callback)(TArray<TAssetType*>, VarTypes...), VarTypes... Vars)
	{
		static_assert(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value, "TAssetType should be derived from UPrimaryDataAsset.");
		auto CallbackLambda = [this, AssetIds, Callback, Caller = MakeWeakObjectPtr(Caller), Vars...]() {
			if (!Caller.IsValid())
			{
				return;
			}

			TArray<TAssetType*> Assets = GetAssetsListFromAssetIds<TAssetType>(AssetIds);
			(Caller.Get()->*Callback)(MoveTemp(Assets), MoveTempIfPossible(Vars)...);
		};
		AsyncLoadAssetsInternal(AssetIds, MoveTemp(CallbackLambda));
	}

	template <typename TAssetType> void AsyncLoadAsset(const FPrimaryAssetId& AssetId, TFunction<void(TAssetType*)>&& Callback)
	{
		static_assert(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value, "TAssetType should be derived from UPrimaryDataAsset.");
		auto CallbackLambda = [this, AssetId, Callback = MoveTemp(Callback)]() {
			if (!Callback)
			{
				return;
			}

			TArray<TAssetType*> Assets = GetAssetsListFromAssetIds<TAssetType>({ AssetId });
			TAssetType* Asset = !Assets.IsEmpty() ? Assets[0] : nullptr;
			Callback(MoveTemp(Asset));
		};
		AsyncLoadAssetsInternal({ AssetId }, MoveTemp(CallbackLambda));
	}

	template <typename TAssetType> void SyncLoadAsset(const FPrimaryAssetId& AssetId, TFunction<void(TAssetType*)>&& Callback)
	{
		static_assert(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value, "TAssetType should be derived from UPrimaryDataAsset.");
		auto CallbackLambda = [this, AssetId, Callback = MoveTemp(Callback)]() {
			if (!Callback)
			{
				return;
			}

			TArray<TAssetType*> Assets = GetAssetsListFromAssetIds<TAssetType>({ AssetId });
			TAssetType* Asset = !Assets.IsEmpty() ? Assets[0] : nullptr;
			Callback(MoveTemp(Asset));
		};
		SyncLoadAssetsInternal({ AssetId }, MoveTemp(CallbackLambda));
	}

	template <typename TAssetType> static void StaticAsyncLoadAsset(const FPrimaryAssetId& AssetId, TFunction<void(TAssetType*)>&& Callback)
	{
		auto* AssetManager = GetCustomizationAssetManager();
		ensure(AssetManager);
		AssetManager->AsyncLoadAsset<TAssetType>(AssetId, MoveTemp(Callback));
	}

	template <typename TAssetType> static void StaticSyncLoadAsset(const FPrimaryAssetId& AssetId, TFunction<void(TAssetType*)>&& Callback)
	{
		auto* AssetManager = GetCustomizationAssetManager();
		ensure(AssetManager);
		AssetManager->SyncLoadAsset<TAssetType>(AssetId, MoveTemp(Callback));
	}

	template <typename TAssetType, typename TCallerType, typename... VarTypes>
	void AsyncLoadAsset(
		const FPrimaryAssetId& AssetId, TCallerType* Caller, void (TCallerType::*Callback)(TAssetType*, VarTypes...), VarTypes... Vars)
	{
		static_assert(TIsDerivedFrom<TAssetType, UPrimaryDataAsset>::Value, "TAssetType should be derived from UPrimaryDataAsset.");
		auto CallbackLambda = [AssetId, Callback, Caller = MakeWeakObjectPtr(Caller), Vars...]() {
			if (!Caller.IsValid())
			{
				return;
			}

			TArray<TAssetType*> Assets = GetAssetsListFromAssetIds<TAssetType>(TArray<FPrimaryAssetId> { AssetId });
			TAssetType* Asset = !Assets.IsEmpty() ? Assets[0] : nullptr;
			(Caller.Get()->*Callback)(MoveTemp(Asset), MoveTempIfPossible(Vars)...);
		};
		AsyncLoadAssetInternal(AssetId, MoveTemp(CallbackLambda));
	}

	template <typename TAssetType, typename TCallerType, typename... VarTypes>
	void StaticAsyncLoadAsset(
		const FPrimaryAssetId& AssetId, TCallerType* Caller, void (TCallerType::*Callback)(TAssetType*, VarTypes...), VarTypes... Vars)
	{
		auto* AssetManager = GetCustomizationAssetManager();
		ensure(AssetManager);
		AssetManager->AsyncLoadAsset<TAssetType>(AssetId, Caller, Callback, MoveTempIfPossible(Vars)...);
	}

	template <typename TCallerType, typename... VarTypes>
	void AsyncLoadAssetClassList(const TArray<FSoftObjectPath>& AssetList, TCallerType* Caller,
		void (TCallerType::*Callback)(TArray<UClass*>, VarTypes...), VarTypes... Vars)
	{
		auto CallbackLambda = [this, AssetList, Callback, Caller = MakeWeakObjectPtr(Caller), Vars...]() {
			if (!Caller.IsValid())
			{
				return;
			}

			TArray<UClass*> Assets = GetAssetsClassList(AssetList);
			(Caller.Get()->*Callback)(MoveTemp(Assets), MoveTempIfPossible(Vars)...);
		};
		AsyncLoadAssetClassListInternal(AssetList, MoveTemp(CallbackLambda));
	}

	UFUNCTION(BlueprintPure)
	TArray<FPrimaryAssetType> GetPrimaryAssetTypes(const TArray<FPrimaryAssetType>& ExcludeList);

	TArray<FPrimaryAssetId> GetAllPrimaryAssetIds();

	FPrimaryAssetId GetPrimaryAssetIdByName(FName InName);

protected:
	void OnBodyPartAssetLoaded(TSharedPtr<FStreamableHandle> LoadHandle, FOnBodyPartLoaded DelegateToCall) const;
	void OnBodyPartAssetListLoaded(TSharedPtr<FStreamableHandle> LoadHandle, FOnBodyPartListLoaded DelegateToCall) const;
	void OnMaterialCustomizationLoaded(TSharedPtr<FStreamableHandle> LoadHandle, FOnMaterialCustomizationLoaded DelegateToCall) const;
	void OnCustomizationAssetLoaded(TSharedPtr<FStreamableHandle> LoadHandle, FOnCustomizationLoaded DelegateToCall) const;
	void OnMaterialPackCustomizationAssetLoaded(TSharedPtr<FStreamableHandle> LoadHandle, FOnMaterialPackLoaded DelegateToCall) const;

private:
	template <typename TCallbackType> void AsyncLoadAssetsInternal(const TArray<FPrimaryAssetId>& AssetIds, TCallbackType&& Callback)
	{
		const TSharedPtr<FStreamableHandle> LoadHandle = LoadPrimaryAssets(AssetIds, TArray<FName>());
		if (!LoadHandle.IsValid())
		{
			Callback();
			return;
		}

		if (LoadHandle->HasLoadCompleted())
		{
			Callback();
			return;
		}

		FStreamableDelegate OnLoadDelegate;
		OnLoadDelegate.BindLambda(Forward<TCallbackType>(Callback));
		LoadHandle->BindCompleteDelegate(OnLoadDelegate);
	}

	template <typename TCallbackType> void SyncLoadAssetsInternal(const TArray<FPrimaryAssetId>& AssetIds, TCallbackType&& Callback)
	{
		for (const auto& AssetId : AssetIds)
		{
			UE_LOG(LogTemp, Display, TEXT("LoadPrimaryAsset: AssetId: %s"), *AssetId.ToString());
			LoadPrimaryAsset<UPrimaryDataAsset>(AssetId);
		}
		Callback();
	}

	template <typename TCallbackType> void AsyncLoadAssetInternal(const FPrimaryAssetId& AssetId, TCallbackType&& Callback)
	{
		AsyncLoadAssetsInternal(TArray<FPrimaryAssetId> { AssetId }, Forward<TCallbackType>(Callback));
	}

	template <typename TCallbackType>
	void AsyncLoadAssetClassListInternal(const TArray<FSoftObjectPath>& AssetList, TCallbackType&& Callback)
	{
		const TSharedPtr<FStreamableHandle> LoadHandle = LoadAssetList(AssetList);

		auto CallbackWithRelease = [LoadHandle, Callback]() {
			if (LoadHandle.IsValid())
			{
				LoadHandle->ReleaseHandle();
			}
			Callback();
		};

		if (!LoadHandle.IsValid())
		{
			Callback();
			return;
		}

		if (LoadHandle->HasLoadCompleted())
		{
			CallbackWithRelease();
			return;
		}

		FStreamableDelegate OnLoadDelegate;
		OnLoadDelegate.BindLambda(CallbackWithRelease);
		LoadHandle->BindCompleteDelegate(OnLoadDelegate);
	}
};
