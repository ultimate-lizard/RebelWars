#include "Controllers/GameplayHumanController.h"

#include "Characters/CombatCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/RWPlayerCameraManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "UI/UIManager.h"

AGameplayHumanController::AGameplayHumanController() :
	Super()
{
	PlayerCameraManagerClass = ARWPlayerCameraManager::StaticClass();

	DeathCameraOffsetLocation = FVector(0.0f, 0.0f, -80.0f);
	DeathCameraOffsetRotation = FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 0.0f));

	SetCameraMode(CameraMode::Default);	
}

void AGameplayHumanController::BeginPlay()
{
	Super::BeginPlay();
}

void AGameplayHumanController::AddViewPunch(FRotator InAngles)
{
	if (ARWPlayerCameraManager* CameraManager = GetRWPlayerCameraManager())
	{
		CameraManager->AddViewPunch(InAngles);
	}
}

FRotator AGameplayHumanController::GetCurrentViewPunchAngle() const
{
	if (ARWPlayerCameraManager* CameraManager = GetRWPlayerCameraManager())
	{
		return CameraManager->GetCurrentViewPunchAngle();
	}

	return FRotator();
}

ARWPlayerCameraManager* AGameplayHumanController::GetRWPlayerCameraManager() const
{
	return Cast<ARWPlayerCameraManager>(PlayerCameraManager);
}

void AGameplayHumanController::Tick(float DeltaTime)
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

	// ServerViewNextPlayer();
}

void AGameplayHumanController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	
}

void AGameplayHumanController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AGameplayHumanController::InitPlayerState()
{
	Super::InitPlayerState();

	// TODO: Implement teams assignment
}

void AGameplayHumanController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindAction(FName("OpenMenu"), EInputEvent::IE_Pressed, this, &AGameplayHumanController::ToggleInGameMenu).bExecuteWhenPaused = true;
	InputComponent->BindAction(FName("OpenTeamSelect"), EInputEvent::IE_Pressed, this, &AGameplayHumanController::ToggleTeamSelect);
}

void AGameplayHumanController::ToggleInGameMenu()
{
	if (UUIManager* UIManager = GetUIManager())
	{
		const bool bNewVisibility = !UIManager->IsGameScreenVisible(GameScreens::InGameMenu);
		UIManager->SetGameScreenVisibility(GameScreens::InGameMenu, bNewVisibility);
		UGameplayStatics::SetGamePaused(GetWorld(), bNewVisibility);
		SetUIInteractionModeEnabled(bNewVisibility);
	}
}

void AGameplayHumanController::ToggleTeamSelect()
{
	if (UUIManager* UIManager = GetUIManager())
	{
		const bool bNewVisibility = !UIManager->IsGameScreenVisible(GameScreens::TeamSelect);
		UIManager->SetGameScreenVisibility(GameScreens::TeamSelect, bNewVisibility);
		SetUIInteractionModeEnabled(bNewVisibility);
	}
}
