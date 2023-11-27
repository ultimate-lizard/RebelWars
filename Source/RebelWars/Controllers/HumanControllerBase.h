#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "HumanControllerBase.generated.h"

class UGameWidgetsData;

UCLASS()
class REBELWARS_API AHumanControllerBase : public APlayerController
{
	GENERATED_BODY()
	
public:
	AHumanControllerBase();

	UFUNCTION(BlueprintPure, Category = "Mouse")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintCallable, Category = "Mouse")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintCallable)
	void SetUIInteractionModeEnabled(bool bEnabled);

	UPROPERTY(Config)
	float MouseSensitivity;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void SetupInputComponent() override;
	virtual void SetPlayer(UPlayer* InPlayer) override;

	UGameWidgetsData* GameWidgetsData;
};
