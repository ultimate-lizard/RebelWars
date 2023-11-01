#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GenericTeamAgentInterface.h"

#include "RWGameModeBase.generated.h"

class ACombatCharacter;

UCLASS()
class REBELWARS_API ARWGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	ARWGameModeBase(const FObjectInitializer& ObjectInitializer);

	virtual void Logout(AController* Exiting) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;

protected:
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;

	UFUNCTION()
	virtual void OnCombatCharacterKilled(AActor* Killer, ACombatCharacter* Victim);

	UFUNCTION()
	virtual void RespawnCombatCharacter(ACombatCharacter* CharacterToRespawn);


	FTimerHandle RespawnTimer;
};
