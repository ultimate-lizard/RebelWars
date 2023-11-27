#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "RWHUD.generated.h"

class UUserWidget;

UCLASS()
class REBELWARS_API ARWHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	ARWHUD();
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UUserWidget> HUDWidgetClass;

private:
	UUserWidget* HUDWidgetInstance;
};
