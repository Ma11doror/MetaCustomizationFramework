// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ESomatotype : uint8
{
	None = 0,
	TypeOne = 1,
	TypeTwo = 2,
	TypeThree = 3,

};
DEFINE_ENUM_TO_STRING(ESomatotype, "SomatotypeEnum")
ENUM_RANGE_BY_FIRST_AND_LAST(ESomatotype, ESomatotype::None, ESomatotype::TypeThree)
