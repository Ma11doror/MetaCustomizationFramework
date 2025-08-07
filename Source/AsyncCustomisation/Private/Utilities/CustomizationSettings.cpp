// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/CustomizationSettings.h"

const UCustomizationSettings* UCustomizationSettings::Get()
{
	return GetDefault<UCustomizationSettings>();
}

UCustomizationSettings* UCustomizationSettings::GetMutable()
{
	return GetMutableDefault<UCustomizationSettings>();
}

bool UCustomizationSettings::GetEnableDebug() const
{
	return bEnableDebug;
}

EMeshMergeMethod UCustomizationSettings::GetMeshMergeMethod() const
{
	return MeshMergeMethod;
}

void UCustomizationSettings::Clear()
{
	CategoryName = TEXT("Customization");
	SectionName = TEXT("General");
}
