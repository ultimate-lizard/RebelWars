#include "Controllers/GameplayHumanController.h"

#include "Characters/CombatCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RWPlayerCameraManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PlayerState.h"
#include "UI/GameWidgetsData.h"
#include "UI/RWHUD.h"
#include "Blueprint/UserWidget.h"
#include "RWGameInstance.h"
#include "GameModes/RWGameModeBase.h"

static bool bWidgetsCreated = false;

AGameplayHumanController::AGameplayHumanController() :
	Super()
{
	PlayerCameraManagerClass = ARWPlayerCameraManager::StaticClass();

	DeathCameraOffsetLocation = FVector(0.0f, 0.0f, -80.0f);
	DeathCameraOffsetRotation = FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 0.0f));

	SetCameraMode(CameraMode::Default);

	InGameMenuWidget = nullptr;
	TeamSelectWidget = nullptr;
}

void AGameplayHumanController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
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

void AGameplayHumanController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UpdateHUDVisibility();
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

void AGameplayHumanController::BeginSpectatingState()
{
	Super::BeginSpectatingState();

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	ClientIgnoreMoveInput(false);
	ClientIgnoreLookInput(false);

	SetCameraMode(CameraMode::Default);

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (URWGameInstance* RWGameInstance = GetGameInstance<URWGameInstance>())
	{
		InGameMenuWidget = RWGameInstance->FindGameWidget(GameScreens::InGameMenu);
		if (InGameMenuWidget)
		{
			InGameMenuWidget->AddToViewport();
			InGameMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		TeamSelectWidget = RWGameInstance->FindGameWidget(GameScreens::TeamSelect);
		if (TeamSelectWidget)
		{
			TeamSelectWidget->AddToViewport();
			TeamSelectWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	UpdateHUDVisibility();

	// TODO: Add precoutions
	ToggleTeamSelect();
}

void AGameplayHumanController::BeginPlayingState()
{
	Super::BeginPlayingState();

	UpdateHUDVisibility();
}

void AGameplayHumanController::ClientSetSpectatorWaiting_Implementation(bool bWaiting)
{
	Super::ClientSetSpectatorWaiting_Implementation(bWaiting);
}

bool AGameplayHumanController::GetHUDPendingVisibility()
{
	if (TeamSelectWidget)
	{
		if (TeamSelectWidget->IsVisible())
		{
			return false;
		}
	}

	if (InGameMenuWidget)
	{
		if (InGameMenuWidget->IsVisible())
		{
			return false;
		}
	}

	if (GetStateName() == NAME_Spectating)
	{
		return false;
	}

	return true;
}

void AGameplayHumanController::UpdateHUDVisibility()
{
	if (ARWHUD* RWHUD = GetHUD<ARWHUD>())
	{
		RWHUD->SetHUDWidgetVisibility(GetHUDPendingVisibility());
	}
}

void AGameplayHumanController::ServerRequestTeamSwitch_Implementation(EAffiliation NewTeam)
{
	RequestTeamSwitch(NewTeam);
}

void AGameplayHumanController::ToggleInGameMenu()
{
	if (TeamSelectWidget)
	{
		if (TeamSelectWidget->IsVisible())
		{
			ToggleScreen(TeamSelectWidget);
			return;
		}
	}

	ToggleScreen(InGameMenuWidget);

	UpdateHUDVisibility();
}

void AGameplayHumanController::ToggleTeamSelect()
{
	if (InGameMenuWidget)
	{
		if (InGameMenuWidget->IsVisible())
		{
			return;
		}
	}

	ToggleScreen(TeamSelectWidget);

	UpdateHUDVisibility();
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

void AGameplayHumanController::RequestTeamSwitch(EAffiliation NewTeam)
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerRequestTeamSwitch(NewTeam);
		return;
	}

	if (GetWorld())
	{
		if (ARWGameModeBase* RWGameMode = GetWorld()->GetAuthGameMode<ARWGameModeBase>())
		{
			RWGameMode->JoinTeam(GetPlayerState<ARWPlayerState>(), NewTeam);
		}
	}
}
