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
}

inline void FCounterComponent::Pop()
{
	if (Counter == 0)
	{
		return;
	}
	--Counter;

	OnTriggered.Broadcast();
}
