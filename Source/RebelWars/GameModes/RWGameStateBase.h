#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "RWGameStateBase.generated.h"

class ACombatCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKillFeed, AActor*, Killer, ACombatCharacter*, Victim);

UCLASS()
class REBELWARS_API ARWGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnKillFeed OnKillFeedDelegate;
};
