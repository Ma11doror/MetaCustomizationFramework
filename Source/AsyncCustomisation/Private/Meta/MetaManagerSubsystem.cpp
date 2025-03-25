#include "Meta/MetaManagerSubsystem.h"

#include "Components/Core/MetaConfig.h"
#include "Kismet/GameplayStatics.h"

UMetaManagerSubsystem* UMetaManagerSubsystem::Get(const UObject* InWorldContextObject, const bool IsRequired)
{
	const auto* GameInstance = UGameplayStatics::GetGameInstance(InWorldContextObject);
	ensure(GameInstance);
	auto* Self = GameInstance->GetSubsystem<UMetaManagerSubsystem>();
	if (IsRequired)
	{
		check(Self);
	}
	return Self;
}

void UMetaManagerSubsystem::Init()
{
	MetaConfigManager->Init();
}

void UMetaManagerSubsystem::Deinit()
{
	// TODO:: add opportunity to clear confing and regenerate it completely
	//MetaConfigManager->Deinit();
}

void UMetaManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMetaManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UMetaManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		TArray<UClass*> ChildClasses;
		GetDerivedClasses(GetClass(), ChildClasses, false);

		// Only create an instance if there is no override implementation defined elsewhere
		return ChildClasses.Num() == 0;
	}
	return false;
}
