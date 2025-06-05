#pragma once

#include "CoreMinimal.h"
#include "BodyPartTypes.generated.h"

UENUM(BlueprintType)
enum class EBodyPartType : uint8
{
	None					= 0,

	Head					= 1,	
	BodySkin	            = 2,
	Legs	                = 3,
	Hands	                = 4,
	Wrists	                = 5,
	Feet	                = 6,
	Beard					= 7,
	Torso					= 8,
	Neck                    = 9,
							
	FaceAccessory           = 10,
	BackAccessoryFirst		= 11,
	BackAccessorySecondary	= 12,

	LegKnife                = 13,
	Hair                    = 14,
	Other                   = 15
};
// DEFINE_ENUM_TO_STRING(EBodyPartType, "BodyPartType")
ENUM_RANGE_BY_FIRST_AND_LAST(EBodyPartType, EBodyPartType::None, EBodyPartType::Other)