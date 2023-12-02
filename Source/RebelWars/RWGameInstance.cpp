#include "RWGameInstance.h"

#include "UI/GameWidgetsData.h"
#include "Blueprint/UserWidget.h"

URWGameInstance::URWGameInstance()
	: Super()
{
	GameWidgetsData = nullptr;

	ConstructorHelpers::FObjectFinder<UGameWidgetsData> FoundGameScreens(TEXT("/Game/UI/GameScreens"));
	if (FoundGameScreens.Succeeded())
	{
		GameWidgetsData = FoundGameScreens.Object;
	}
}

UUserWidget* URWGameInstance::FindGameWidget(FName InName)
{
	return GameWidgetInstances.FindRef(InName);
}

void URWGameInstance::Init()
{
	Super::Init();

	for (auto Pair : GameWidgetsData->WidgetsClasses)
	{
		if (UUserWidget* NewWidget = CreateWidget(this, Pair.Value, Pair.Key))
		{
			GameWidgetInstances.Emplace(Pair.Key, NewWidget);
		}
	}

	UE_LOG(LogTemp, Log, TEXT(""));
}

ULocalPlayer* URWGameInstance::CreateInitialPlayer(FString& OutError)
{
	ULocalPlayer* NewLocalPlayer = Super::CreateInitialPlayer(OutError);

	for (auto Pair : GameWidgetInstances)
	{
		if (UUserWidget* NewWidget = Pair.Value)
		{
			NewWidget->SetOwningLocalPlayer(NewLocalPlayer);
		}
	}

	return NewLocalPlayer;
}
