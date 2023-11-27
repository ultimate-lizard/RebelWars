#include "Controllers/MainMenuHumanController.h"

#include "UI/UIManager.h"

void AMainMenuHumanController::BeginPlay()
{
	Super::BeginPlay();

	if (UUIManager* UIManager = GetUIManager())
	{
		UIManager->SetGameScreenVisibility(GameScreens::MainMenu, true);
	}
	
	SetUIInteractionModeEnabled(true);
}

void AMainMenuHumanController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	SetUIInteractionModeEnabled(false);
}
