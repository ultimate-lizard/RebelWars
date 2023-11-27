#include "Controllers/MainMenuHumanController.h"

#include "UI/GameWidgetsData.h"
#include "Blueprint/UserWidget.h"

AMainMenuHumanController::AMainMenuHumanController()
	: Super()
{
}

void AMainMenuHumanController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (TSubclassOf<UUserWidget> MainMenuWidgetClass = GameWidgetsData->WidgetsClasses.FindRef(GameScreens::MainMenu))
	{
		MainMenuWidget = CreateWidget(GetGameInstance(), MainMenuWidgetClass);
	}
}

void AMainMenuHumanController::BeginPlay()
{
	Super::BeginPlay();

	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport();
	}
	
	SetUIInteractionModeEnabled(true);
}

void AMainMenuHumanController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	SetUIInteractionModeEnabled(false);
}
