#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CustomizationSettings.generated.h"


UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Customization Settings"))
class ASYNCCUSTOMISATION_API UCustomizationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
protected:

	// Enable debugging options
	
	UPROPERTY(config, EditAnywhere, Category = "Customization Settings")
	bool bEnableDebug = false;\
	
public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Customization Settings"))
	static const UCustomizationSettings* Get();
	static UCustomizationSettings* GetMutable();

	[[nodiscard]] bool GetEnableDebug() const;
	void Clear();
};
