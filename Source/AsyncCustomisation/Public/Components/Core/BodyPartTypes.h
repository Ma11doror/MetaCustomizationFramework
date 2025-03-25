#pragma once

#include "CoreMinimal.h"
#include "BodyPartTypes.generated.h"

UENUM(BlueprintType)
enum class EBodyPartType : uint8
{
	None					= 0,
	
	BodySkin	            = 1,
	Legs	                = 2,
	Hands	                = 3,
	Wrists	                = 4,
	Feet	                = 5,
	Beard					= 6,
	Torso					= 7,
	Neck                    = 8,
	
	FaceAccessory           = 9,
	BackAccessoryFirst		= 10,
	BackAccessorySecondary	= 11,

	LegKnife                = 12,
	Hair                    = 13,
	Other                   = 14,
};
//DEFINE_ENUM_TO_STRING(EBodyPartType)
ENUM_RANGE_BY_FIRST_AND_LAST(EBodyPartType, EBodyPartType::None, EBodyPartType::Other)