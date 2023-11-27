#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Utils/TeamStatics.h"

#include "RWGameStateBase.generated.h"

class ACombatCharacter;
class ARWPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKillFeed, AActor*, Killer, ACombatCharacter*, Victim);

UCLASS()
class REBELWARS_API ARWGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	ARWGameStateBase();

	void AddPlayerToTeam(ARWPlayerState* InPlayerState, EAffiliation InTeam);

	void SetNumTeams(int32 InNumTeams);

	UFUNCTION(BlueprintPure)
	int32 GetNumTeams() const;

	UPROPERTY(BlueprintAssignable)
	FOnKillFeed OnKillFeedDelegate;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, Transient)
	int32 NumTeams;
};
