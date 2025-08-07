#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CustomizationSettings.generated.h"

UENUM(BlueprintType)
enum class EMeshMergeMethod : uint8
{
	
	MasterPose UMETA(DisplayName = "Master Pose"),
	SyncMeshMerge UMETA(DisplayName = "Synch Skeletal Mesh Merge"),

	// /** Three-phase asynchronous merger*/
	// AsyncMeshMerge UMETA(DisplayName = "Three-Phase Async Mesh Merge")
};

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Customization Settings"))
class ASYNCCUSTOMISATION_API UCustomizationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
protected:

	// Enable debugging options
	
	UPROPERTY(config, EditAnywhere, Category = "Customization Settings")
	bool bEnableDebug = false;
	
	UPROPERTY(config, EditAnywhere, Category = "Customization Settings|Mesh Merge", 
		meta = (DisplayName = "Mesh Merge Method", 
		       ToolTip = "Choose method how customization will be working."))
	EMeshMergeMethod MeshMergeMethod = EMeshMergeMethod::SyncMeshMerge;
	
public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Customization Settings"))
	static const UCustomizationSettings* Get();
	static UCustomizationSettings* GetMutable();

	[[nodiscard]] bool GetEnableDebug() const;
	[[nodiscard]] EMeshMergeMethod GetMeshMergeMethod() const;
	void Clear();
};
