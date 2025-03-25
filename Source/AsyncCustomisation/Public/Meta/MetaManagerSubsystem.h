#pragma once

#include "Constants/GlobalConstants.h"
#include "MetaManagerSubsystem.generated.h"

class UMetaConfig;
using namespace GLOBAL_CONSTANTS;


UCLASS(BlueprintType)
class UMetaManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FMetaManagerEvent, const bool /* IsSuccess */)
	FMetaManagerEvent OnInitialized;

public:
	static UMetaManagerSubsystem* Get(const UObject* InWorldContextObject, const bool IsRequired = false);
	void Init();
	void Deinit();

	//Subsystem interface begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//Subsystem interface end

public:


	UPROPERTY()
	UMetaConfig* MetaConfigManager = nullptr;

	
private:

	//void OnInventoryManagerInitialized();
	//void OnMetaConfigInitialized(const bool IsSuccess);
};
