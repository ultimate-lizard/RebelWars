#include "Controllers/MainMenuHumanController.h"

#include "UI/GameWidgetsData.h"
#include "Blueprint/UserWidget.h"
#include "RWGameInstance.h"

AMainMenuHumanController::AMainMenuHumanController()
	: Super()
{
}

void AMainMenuHumanController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMainMenuHumanController::BeginPlay()
{
	Super::BeginPlay();

	if (URWGameInstance* RWGameInstance = GetGameInstance<URWGameInstance>())
	{
		if (UUserWidget* MainMenuWidget = RWGameInstance->FindGameWidget(GameScreens::MainMenu))
		{
			MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
			MainMenuWidget->AddToViewport();
		}
	}
	
	SetUIInteractionModeEnabled(true);
}

void AMainMenuHumanController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	SetUIInteractionModeEnabled(false);
}
