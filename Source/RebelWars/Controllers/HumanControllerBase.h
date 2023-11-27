#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "HumanControllerBase.generated.h"

class UGameScreens;
class UUserWidget;
class UUIManager;

namespace GameScreens
{
	static const FName MainMenu(TEXT("MainMenu"));
	static const FName InGameMenu(TEXT("InGameMenu"));
	static const FName TeamSelect(TEXT("TeamSelect"));
}

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

	// void SetGameScreenVisibility(FName GameScreenName, bool bVisible);
	void SetUIInteractionModeEnabled(bool bEnabled);

	// bool IsGameScreenVisible(FName GameScreenName) const;

	UPROPERTY(Config)
	float MouseSensitivity;

	UUIManager* GetUIManager();

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void SetupInputComponent() override;
	virtual void SetPlayer(UPlayer* InPlayer) override;
};
