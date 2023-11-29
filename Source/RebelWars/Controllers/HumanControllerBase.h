#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "HumanControllerBase.generated.h"

class UGameWidgetsData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHumanPlayerRestart);

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

	UPROPERTY(BlueprintAssignable)
	FOnHumanPlayerRestart OnHumanPlayerRestartDelegate;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void SetupInputComponent() override;
	virtual void SetPlayer(UPlayer* InPlayer) override;
	virtual void BeginPlayingState() override;
	virtual void OnPossess(APawn* aPawn) override;
	virtual void OnUnPossess() override;

	UGameWidgetsData* GameWidgetsData;

private:
	UPROPERTY(Transient)
	APawn* PreviousPawn;
};
