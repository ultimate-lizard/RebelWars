#include "Controllers/HumanPlayerController.h"

#include "Characters/CombatCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/RWPlayerCameraManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerState.h"

AHumanPlayerController::AHumanPlayerController(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ARWPlayerCameraManager::StaticClass();

	DeathCameraOffsetLocation = FVector(0.0f, 0.0f, -80.0f);
	DeathCameraOffsetRotation = FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 0.0f));

	SetCameraMode(CameraMode::Default);
}

void AHumanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ChangeState(NAME_Spectating);
	PlayerState->bIsSpectator = true;
	PlayerState->bOnlySpectator = true;
}

void AHumanPlayerController::AddViewPunch(FRotator InAngles)
{
	if (ARWPlayerCameraManager* CameraManager = GetRWPlayerCameraManager())
	{
		CameraManager->AddViewPunch(InAngles);
	}
}

FRotator AHumanPlayerController::GetCurrentViewPunchAngle() const
{
	if (ARWPlayerCameraManager* CameraManager = GetRWPlayerCameraManager())
	{
		return CameraManager->GetCurrentViewPunchAngle();
	}

	return FRotator();
}

ARWPlayerCameraManager* AHumanPlayerController::GetRWPlayerCameraManager() const
{
	return Cast<ARWPlayerCameraManager>(PlayerCameraManager);
}

void AHumanPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (PlayerCameraManager)
	{
		if (ACombatCharacter* CombatPawn = GetPawn<ACombatCharacter>())
		{
			if (PlayerCameraManager->CameraStyle == CameraMode::Default && CombatPawn->IsDead())
			{
				SetCameraMode(CameraMode::FreeCam);
			}
			else if (PlayerCameraManager->CameraStyle == CameraMode::FreeCam && !CombatPawn->IsDead())
			{
				SetCameraMode(CameraMode::Default);
			}
		}
	}

	ServerViewNextPlayer();
}

void AHumanPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//if (ASpectatorPawn* Spec = SpawnSpectatorPawn())
	//{
	//	ServerViewNextPlayer();
	//}
}
