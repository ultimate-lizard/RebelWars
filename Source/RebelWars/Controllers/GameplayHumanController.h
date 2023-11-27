#pragma once

#include "CoreMinimal.h"
#include "Controllers/HumanControllerBase.h"

#include "GameplayHumanController.generated.h"

class ACombatCharacter;
class ARWPlayerCameraManager;

namespace CameraMode
{
	static const FName FreeCam(TEXT("FreeCam"));
	static const FName Default(TEXT("Default"));
}

UCLASS()
class REBELWARS_API AGameplayHumanController : public AHumanControllerBase
{
	GENERATED_BODY()
	
public:
	AGameplayHumanController();
	virtual void BeginPlay() override;

	void AddViewPunch(FRotator InRotator);
	FRotator GetCurrentViewPunchAngle() const;

	ARWPlayerCameraManager* GetRWPlayerCameraManager() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInGameMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleTeamSelect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FVector DeathCameraOffsetLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FRotator DeathCameraOffsetRotation;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void InitPlayerState() override;
	virtual void SetupInputComponent() override;

	FVector CurrentDeathCameraLocation;
};
