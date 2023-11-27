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
	ARWGameStateBase();

	void SetNumTeams(int32 InNumTeams);

	UFUNCTION(BlueprintPure)
	int32 GetNumTeams() const;

	UPROPERTY(BlueprintAssignable)
	FOnKillFeed OnKillFeedDelegate;

private:
	int32 NumTeams;
};
