#include "Controllers/GameplayHumanController.h"

#include "Characters/CombatCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RWPlayerCameraManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerState.h"
#include "UI/GameWidgetsData.h"
#include "Blueprint/UserWidget.h"

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

	if (InGameMenuWidget)
	{
		InGameMenuWidget->AddToViewport();
		InGameMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TeamSelectWidget)
	{
		TeamSelectWidget->AddToViewport();
		TeamSelectWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
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
}

void AGameplayHumanController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GameWidgetsData)
	{
		if (TSubclassOf<UUserWidget> InGameMenuClass = GameWidgetsData->WidgetsClasses.FindRef(GameScreens::InGameMenu))
		{
			InGameMenuWidget = CreateWidget(GetGameInstance(), InGameMenuClass);
		}

		if (TSubclassOf<UUserWidget> TeamSelectClass = GameWidgetsData->WidgetsClasses.FindRef(GameScreens::TeamSelect))
		{
			TeamSelectWidget = CreateWidget(GetGameInstance(), TeamSelectClass);
		}
	}
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
	ToggleScreen(InGameMenuWidget);
}

void AGameplayHumanController::ToggleTeamSelect()
{
	ToggleScreen(TeamSelectWidget);
}

void AGameplayHumanController::ToggleScreen(UUserWidget* GameScreenWidget)
{
	if (!GameScreenWidget)
	{
		return;
	}

	bool bNewVisibility = !GameScreenWidget->IsVisible();
	GameScreenWidget->SetVisibility(bNewVisibility ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetUIInteractionModeEnabled(bNewVisibility);
}
