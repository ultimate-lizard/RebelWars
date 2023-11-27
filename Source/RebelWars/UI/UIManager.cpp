#include "UI/UIManager.h"

#include "UI/GameScreens.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

UUIManager::UUIManager()
	: Super()
{
	GameScreens = nullptr;

	ConstructorHelpers::FObjectFinder<UGameScreens> FoundGameScreens(TEXT("/Game/UI/GameScreens"));
	if (FoundGameScreens.Succeeded() && FoundGameScreens.Object)
	{
		GameScreens = FoundGameScreens.Object;
	}
}

void UUIManager::CreateWidgets()
{
	if (GameScreens)
	{
		for (auto Pair : GameScreens->GameScreens)
		{
			if (Pair.Value)
			{
				UUserWidget* GameScreenInstance = CreateWidget(UGameplayStatics::GetGameInstance(GetWorld()), Pair.Value);
				if (GameScreenInstance)
				{
					// GameScreenInstance->AddToViewport();
					GameScreenInstance->SetVisibility(ESlateVisibility::Collapsed);

					GameScreensInstances.Emplace(Pair.Key, GameScreenInstance);
				}
			}
		}
	}
}

bool UUIManager::IsGameScreenVisible(FName GameScreenName) const
{
	if (const UUserWidget* ScreenWidget = GameScreensInstances.FindRef(GameScreenName))
	{
		return ScreenWidget->GetVisibility() != ESlateVisibility::Collapsed && ScreenWidget->IsInViewport();
	}

	return false;
}

void UUIManager::SetGameScreenVisibility(FName GameScreenName, bool bVisible)
{
	if (UUserWidget* ScreenWidget = GameScreensInstances.FindRef(GameScreenName))
	{
		ScreenWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (!ScreenWidget->IsInViewport() && bVisible)
		{
			ScreenWidget->AddToViewport();
		}
	}
}
