#pragma once
#include "CoreMinimal.h"

#include "CounterComponent.generated.h"

USTRUCT()
struct FCounterComponent
{
	GENERATED_BODY()

public:
	TMulticastDelegate<void()> OnTriggered;

	void Push();
	void Pop();

private:
	uint64 Counter = 0;
};

inline void FCounterComponent::Push()
{
	if (Counter == TNumericLimits<int64>::Max())
	{
		return;
	}
	++Counter;
	UE_LOG(LogTemp, Warning, TEXT("FCounterComponent: PUSHED. New Counter: %llu"), Counter);
}

inline void FCounterComponent::Pop()
{
	if (Counter > 0)
	{
		Counter--;
		UE_LOG(LogTemp, Warning, TEXT("FCounterComponent: POPPED. New Counter: %d."), Counter);
		if (Counter == 0) 
		{
			UE_LOG(LogTemp, Warning, TEXT("FCounterComponent: Counter is ZERO. Broadcasting OnTriggered."));
			OnTriggered.Broadcast();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FCounterComponent: POPPED when counter was already zero or less!"));
	}
}
