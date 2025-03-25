// Fill out your copyright notice in the Description page of Project Settings.


#include "Meta/GameInstanceBase.h"

#include "Meta/MetaManagerSubsystem.h"

void UGameInstanceBase::InitializeMeta()
{
	GetSubsystem<UMetaManagerSubsystem>()->Deinit();

	GetSubsystem<UMetaManagerSubsystem>()->Init();
}
