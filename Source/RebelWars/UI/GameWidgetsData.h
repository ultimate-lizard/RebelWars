#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameWidgetsData.generated.h"

namespace GameScreens
{
	static const FName MainMenu(TEXT("MainMenu"));
	static const FName InGameMenu(TEXT("InGameMenu"));
	static const FName TeamSelect(TEXT("TeamSelect"));
}

UCLASS()
class REBELWARS_API UGameWidgetsData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, TSubclassOf<UUserWidget>> WidgetsClasses;
};
