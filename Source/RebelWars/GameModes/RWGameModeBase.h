#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GenericTeamAgentInterface.h"
#include "Utils/TeamStatics.h"

#include "RWGameModeBase.generated.h"

class ACombatCharacter;
class ARWPlayerState;

UCLASS()
class REBELWARS_API ARWGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	ARWGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual bool MustSpectate_Implementation(APlayerController* NewPlayerController) const override;

	int32 GetBotDifficulty() const;

	UFUNCTION(BlueprintCallable)
	void JoinTeam(ARWPlayerState* PlayerState, EAffiliation Team);

	UFUNCTION(BlueprintPure, Category = "Teams")
	bool IsTeamSelectAllowed() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSet<EAffiliation> AvailableTeams;

protected:
	virtual void StartMatch() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION()
	virtual void OnCombatCharacterKilled(AActor* Killer, ACombatCharacter* Victim);

	UFUNCTION()
	virtual void RespawnCombatCharacter(ACombatCharacter* CharacterToRespawn);

	int32 BotsSpawned;
	
	TMap<int32, FTimerHandle> RespawnTimers;

	bool bBotsEnabled;
	int32 BotCount;
	int32 BotDifficulty;

	bool bAllowTeamSelect : 1;
};
