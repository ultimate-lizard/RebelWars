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

	virtual void EndSpectatingOnly();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInGameMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleTeamSelect();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleScreen(UUserWidget* GameScreenWidget);

	UFUNCTION(BlueprintCallable, Category = "Team")
	void RequestTeamSwitch(EAffiliation NewTeam);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FVector DeathCameraOffsetLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	FRotator DeathCameraOffsetRotation;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void InitPlayerState() override;
	virtual void SetupInputComponent() override;
	virtual void BeginSpectatingState() override;
	virtual void BeginPlayingState() override;

	virtual void OnRep_Pawn() override;
	virtual void OnRep_PlayerState() override;

	bool GetHUDPendingVisibility();
	void UpdateHUDVisibility();
	void InitWidgets();

	UFUNCTION(Server, Reliable)
	virtual void ServerRequestTeamSwitch(EAffiliation NewTeam);
	virtual void ServerRequestTeamSwitch_Implementation(EAffiliation NewTeam);

	FVector CurrentDeathCameraLocation;

private:
	UUserWidget* InGameMenuWidget;
	UUserWidget* TeamSelectWidget;
};
