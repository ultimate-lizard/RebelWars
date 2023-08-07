#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HumanPlayerController.generated.h"

UCLASS()
class REBELWARS_API AHumanPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	void AddViewPunch(FRotator InRotator);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	float ViewPunchAlpha = 1.0f;
	FRotator ViewPunchAngle;
	FRotator ViewPunchSource;
	bool bFinishedPunch = false;
};
