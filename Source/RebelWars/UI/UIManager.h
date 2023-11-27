#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "UIManager.generated.h"

class UGameScreens;

UCLASS()
class REBELWARS_API UUIManager : public UObject
{
	GENERATED_BODY()
	
public:
	UUIManager();

	void CreateWidgets();

	void SetGameScreenVisibility(FName GameScreenName, bool bVisible);
	bool IsGameScreenVisible(FName GameScreenName) const;

private:
	UPROPERTY(Transient)
	TMap<FName, UUserWidget*> GameScreensInstances;

	UGameScreens* GameScreens;
};
