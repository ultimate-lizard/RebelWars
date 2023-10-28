// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "RWPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class REBELWARS_API ARWPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	ARWPlayerCameraManager();
	virtual void UpdateCamera(float DeltaTime) override;

	void AddViewPunch(FRotator InAngles);
	FRotator GetCurrentViewPunchAngle() const;

protected:
	void DecayViewPunch(float DeltaTime);

	float ViewPunchAlpha;
	FRotator ViewPunchAngle;
	bool bFinishedPunch;
};
