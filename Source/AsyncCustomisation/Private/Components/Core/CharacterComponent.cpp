#include "BaseCharacter.h"
#include "AsyncCustomisation/Public/Components/Core/CharacterComponentBase.h"

UCharacterComponentBase::UCharacterComponentBase()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = false;
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	UpdateOwner();
}

void UCharacterComponentBase::Activate(bool bReset)
{
	if(bReset)
	{
		UpdateOwner();
	}
	Super::Activate(bReset);
}

void UCharacterComponentBase::Deactivate()
{
	Super::Deactivate();
}

void UCharacterComponentBase::BindCharacter(ABaseCharacter* InCharacter)
{
	//TODO:: need metamodel?
}

void UCharacterComponentBase::UnbindCharacter(ABaseCharacter* InCharacter)
{
	if (!ensureAlwaysMsgf(InCharacter, TEXT("Invalid Character")))
	{
		return;
	}
	//TODO::
}

void UCharacterComponentBase::UpdateOwner()
{
	if (OwningCharacter.IsValid())
	{
		UnbindCharacter(OwningCharacter.Get());
	}
	auto* Owner = GetOwner();

	if (auto* Character = Cast<ABaseCharacter>(Owner))
	{
		OwningCharacter = Character;
	}
	if (OwningCharacter.IsValid())
	{
		BindCharacter(OwningCharacter.Get());
	}
}

ABaseCharacter* UCharacterComponentBase::GetOwningCharacter() const
{
	return OwningCharacter.IsValid() ? OwningCharacter.Get() : nullptr;
}
