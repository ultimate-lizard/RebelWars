#pragma once

#include "CoreMinimal.h"
#include "Controllers/HumanControllerBase.h"

#include "MainMenuHumanController.generated.h"

UCLASS()
class REBELWARS_API AMainMenuHumanController : public AHumanControllerBase
{
	GENERATED_BODY()

public:
	AMainMenuHumanController();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	UUserWidget* MainMenuWidget;
};
