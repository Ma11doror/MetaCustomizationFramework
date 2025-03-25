// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncCustomisation/Public/Utilities/TimerComponent.h"

void UTimerComponent::Start(const float InDelay, const int32 InRepeats, const ETimerMode InMode)
{
	InitialDelay = Delay = RemainingDelay = InDelay;
	InitialRepeats = RemainingRepeats = IsLooped ? 1 : InRepeats;
	InitialMode = Mode = InMode;
	IsLooped = InRepeats == 0;
	Mode = InMode;
	bIsRunning = true;
}

void UTimerComponent::Stop()
{
	bIsRunning = false;
}

void UTimerComponent::Restart()
{
	Stop();
	Start(InitialDelay, InitialRepeats, InitialMode);
}

bool UTimerComponent::IsRunning() const
{
	return bIsRunning;
}

bool UTimerComponent::IsTickable() const
{
	return FTickableGameObject::IsTickable();
}

TStatId UTimerComponent::GetStatId() const
{
	return TStatId();
}

void UTimerComponent::Tick(float DeltaTime)
{
	if (!IsRunning())
	{
		return;
	}

	RemainingDelay -= (Mode == ETimerMode::TimeDelay) ? DeltaTime : 1.0f;

	if (RemainingDelay > 0.0f)
	{
		return;
	}

	OnTimerElapsed.Broadcast();

	if (!IsLooped)
	{
		RemainingRepeats--;
		if (RemainingRepeats == 0)
		{
			Stop();
		}
	}

	// Setup delay again
	if (RemainingRepeats > 0)
	{
		RemainingDelay = Delay; // reset time for next repeat
	}
}
