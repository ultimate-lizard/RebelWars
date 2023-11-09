#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RWHumanControllerBase.generated.h"

UCLASS()
class REBELWARS_API ARWHumanControllerBase : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Mouse")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintCallable, Category = "Mouse")
	void SetMouseSensitivity(float NewSensitivity);

	UPROPERTY(Config)
	float MouseSensitivity;
};
