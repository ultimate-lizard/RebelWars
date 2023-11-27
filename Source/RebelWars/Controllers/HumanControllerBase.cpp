#include "Controllers/HumanControllerBase.h"

#include "GameFramework/PlayerInput.h"
#include "Blueprint/UserWidget.h"
#include "UI/GameScreens.h"
#include "RWGameInstance.h"

AHumanControllerBase::AHumanControllerBase()
	: Super()
{

}

UUIManager* AHumanControllerBase::GetUIManager()
{
	if (URWGameInstance* RWGameInstance = GetGameInstance<URWGameInstance>())
	{
		return RWGameInstance->GetUIManager();
	}

	return nullptr;
}

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

void AHumanControllerBase::SetUIInteractionModeEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		if (APawn* ControlledPawn = GetPawn())
		{
			ControlledPawn->DisableInput(this);
		}

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
	}
	else
	{
		if (APawn* ControlledPawn = GetPawn())
		{
			ControlledPawn->EnableInput(this);
		}

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
	}
}