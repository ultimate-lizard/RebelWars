#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameScreens.generated.h"

UCLASS()
class REBELWARS_API UGameScreens : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, TSubclassOf<UUserWidget>> GameScreens;
};
