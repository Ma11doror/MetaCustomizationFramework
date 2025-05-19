#pragma once

#include "CoreMinimal.h"
#include "MetaModelBase.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class UMetaModelBase : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInvalidateNative, UMetaModelBase* /*InModel*/);
	FORCEINLINE FOnInvalidateNative& OnInvalidateNativeEvent() { return OnInvalidateNative; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvalidate, UMetaModelBase*, InModel);
	UPROPERTY(BlueprintAssignable)
	FOnInvalidate OnInvalidate;

public:
	virtual void Invalidate()
	{
		OnInvalidateNative.Broadcast(this);
		OnInvalidate.Broadcast(this);
	}

protected:
	FOnInvalidateNative OnInvalidateNative;
};
