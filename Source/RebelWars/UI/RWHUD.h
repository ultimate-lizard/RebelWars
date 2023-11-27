#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "RWHUD.generated.h"

UCLASS()
class REBELWARS_API ARWHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	ARWHUD();

	UFUNCTION(BlueprintCallable)
	void SetHUDWidgetVisibility(bool bVisibility);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UUserWidget> HUDWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	UUserWidget* HUDWidgetInstance;
};
