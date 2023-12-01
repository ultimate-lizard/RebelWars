#include "Controllers/HumanControllerBase.h"

#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpectatorPawn.h"
#include "Blueprint/UserWidget.h"
#include "UI/GameWidgetsData.h"

void AHumanControllerBase::BeginPlay()
{
	Super::BeginPlay();

	SetMouseSensitivity(GetMouseSensitivity());
}

void AHumanControllerBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

float AHumanControllerBase::GetMouseSensitivity() const
{
	float Sensitivity = 0.1f;
	GConfig->GetFloat(TEXT("/Script/RebelWars.RWPlayerInput"), TEXT("MouseSensitivity"), Sensitivity, GInputIni);

	return Sensitivity;
}

void AHumanControllerBase::SetMouseSensitivity(float NewSensitivity)
{
	if (PlayerInput)
	{
		PlayerInput->SetMouseSensitivity(NewSensitivity);
	}

	GConfig->SetFloat(TEXT("/Script/RebelWars.RWPlayerInput"), TEXT("MouseSensitivity"), NewSensitivity, GInputIni);
}

void AHumanControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}
}

void AHumanControllerBase::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);
}

void AHumanControllerBase::BeginPlayingState()
{
	Super::BeginPlayingState();

	OnHumanPlayerRestartDelegate.Broadcast();
}

void AHumanControllerBase::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	if (PreviousPawn)
	{
		PreviousPawn->Destroy();
	}
}

void AHumanControllerBase::OnUnPossess()
{
	PreviousPawn = GetPawn();

	Super::OnUnPossess();
}

void AHumanControllerBase::SetUIInteractionModeEnabled(bool bEnabled)
{
	APawn* ControlledPawn = GetPawn();
	ASpectatorPawn* ControlledSpectatorPawn = GetSpectatorPawn();

	if (bEnabled)
	{
		if (ControlledPawn)
		{
			ControlledPawn->DisableInput(this);
		}
		
		if (ControlledSpectatorPawn)
		{
			ControlledSpectatorPawn->DisableInput(this);
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);

		SetInputMode(InputMode);
		SetShowMouseCursor(true);
	}
	else
	{
		if (ControlledPawn)
		{
			ControlledPawn->EnableInput(this);
		}

		if (ControlledSpectatorPawn)
		{
			ControlledSpectatorPawn->EnableInput(this);
		}

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);

		SetShowMouseCursor(false);
	}
}
