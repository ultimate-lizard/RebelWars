#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "RWGameInstance.generated.h"

class UUIManager;

UCLASS()
class REBELWARS_API URWGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UUIManager* GetUIManager();

protected:
	virtual void Init() override;

private:
	UPROPERTY(Transient)
	UUIManager* UIManager;
};
