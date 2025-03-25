// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimerComponent.generated.h"


UCLASS()
class ASYNCCUSTOMISATION_API UTimerComponent : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	enum class ETimerMode : uint8
	{
		TimeDelay,
		FrameDelay,
	};

public:
	TMulticastDelegate<void()> OnTimerElapsed;

	void Start(const float InDelay = 1.0f, const int32 InRepeats = 1, const ETimerMode InMode = ETimerMode::TimeDelay);
	void Stop();
	void Restart();
	bool IsRunning() const;

	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

private:
	float Delay = 1.0f;
	float RemainingDelay = 0.f;
	int32 RemainingRepeats = 0;
	bool bIsRunning = false;
	bool IsLooped = false;

	ETimerMode Mode = ETimerMode::TimeDelay;

private:
	float InitialDelay = Delay;
	float InitialRepeats = RemainingRepeats;
	ETimerMode InitialMode = Mode;
};
