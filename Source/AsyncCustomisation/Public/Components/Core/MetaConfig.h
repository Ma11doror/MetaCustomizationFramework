// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MetaModelBase.h"
#include "UObject/Object.h"
#include "MetaConfig.generated.h"


class UMetaConfigItemData;

UCLASS(Abstract, BlueprintType, Blueprintable)
class ASYNCCUSTOMISATION_API UMetaConfig : public UMetaModelBase
{
	GENERATED_BODY()
public:
	
	static UMetaConfig* Get(const UObject* InWorldContextObject, const bool IsRequired = false);

	static const UMetaConfigItemData* GetItemConfigBySlug(const UObject* WCO, const FName& InSlug);
	const UMetaConfigItemData* GetItemConfigByPrimaryAssetId(const FPrimaryAssetId& InAssetId);

	void Init();

private:
	
	UPROPERTY()
	TMap<FName, UMetaConfigItemData*> AllItemsConfig;
};
