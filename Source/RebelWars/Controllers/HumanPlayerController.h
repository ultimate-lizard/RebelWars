#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "HumanPlayerController.generated.h"

class ACombatCharacter;
class ARWPlayerCameraManager;

namespace CameraMode
{
	static const FName FreeCam(TEXT("FreeCam"));
	static const FName Default(TEXT("Default"));
}

UCLASS()
class REBELWARS_API AHumanPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AHumanPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void AddViewPunch(FRotator InRotator);
	FRotator GetCurrentViewPunchAngle() const;

	ARWPlayerCameraManager* GetRWPlayerCameraManager() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FVector DeathCameraOffsetLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FRotator DeathCameraOffsetRotation;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	FVector CurrentDeathCameraLocation;
};
