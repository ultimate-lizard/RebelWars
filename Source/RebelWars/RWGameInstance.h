#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "RWGameInstance.generated.h"

class UGameWidgetsData;

UCLASS()
class REBELWARS_API URWGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	URWGameInstance();

	UUserWidget* FindGameWidget(FName InName);

protected:
	virtual void Init() override;
	virtual ULocalPlayer* CreateInitialPlayer(FString& OutError) override;

private:
	UGameWidgetsData* GameWidgetsData;

	UPROPERTY(Transient)
	TMap<FName, UUserWidget*> GameWidgetInstances;
};
