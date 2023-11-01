#include "GameModes/RWGameModeBase.h"

#include <GameFramework/PlayerState.h>
#include <Characters/CombatCharacter.h>
#include <GameModes/RWGameStateBase.h>
#include <Controllers/CombatAIController.h>
#include "Kismet/GameplayStatics.h"

APawn* ARWGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	APawn* SpawnedPawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(SpawnedPawn))
	{
		CombatCharacter->OnKillDelegate.AddDynamic(this, &ARWGameModeBase::OnCombatCharacterKilled);
	}

	return SpawnedPawn;
}

void ARWGameModeBase::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer && NewPlayer->StartSpot == nullptr)
	{
		NewPlayer->StartSpot = FindPlayerStart(NewPlayer);
	}

	Super::RestartPlayer(NewPlayer);
}

ARWGameModeBase::ARWGameModeBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// GameStateClass = ARWGameStateBase::StaticClass();
}

void ARWGameModeBase::Logout(AController* Exiting)
{
	if (Exiting)
	{
		if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(Exiting->GetPawn()))
		{
			CombatCharacter->OnKillDelegate.RemoveAll(this);
		}
	}

	return Super::Logout(Exiting);
}

void ARWGameModeBase::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	if (APawn* NewPlayerPawn = NewPlayer->GetPawn())
	{
		Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

		if (StartSpot)
		{
			NewPlayer->SetControlRotation(StartSpot->GetActorRotation());
			NewPlayerPawn->TeleportTo(StartSpot->GetActorLocation(), StartSpot->GetActorRotation());
		}
	}
	else
	{
		Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
	}
}

void ARWGameModeBase::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();
	
	if (UWorld* World = GetWorld())
	{
		if (auto DefaultPawnObject = DefaultPawnClass.GetDefaultObject())
		{
			if (auto DefaultAIControllerObject = DefaultPawnObject->AIControllerClass.GetDefaultObject())
			{
				auto SpawnedAIController = World->SpawnActor(DefaultAIControllerObject->GetClass());
			}
		}
		
	}
}

void ARWGameModeBase::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	for (FConstControllerIterator Iter = GetWorld()->GetControllerIterator(); Iter; ++Iter)
	{
		if (ACombatAIController* AI = Cast<ACombatAIController>(*Iter))
		{
			RestartPlayer(AI);
		}
	}
	// for ( ())
}

void ARWGameModeBase::OnCombatCharacterKilled(AActor* Killer, ACombatCharacter* Victim)
{
	if (APlayerState* KilledPlayer = Victim->GetPlayerState())
	{
		const FString CharName = KilledPlayer->GetPlayerName() + TEXT(" has been killed");
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, *CharName);

		if (GetWorldTimerManager().IsTimerActive(RespawnTimer))
		{
			GetWorldTimerManager().ClearTimer(RespawnTimer);
		}

		FTimerDelegate RespawnTimerDelegate = FTimerDelegate::CreateUObject(this, &ARWGameModeBase::RespawnCombatCharacter, Victim);
		GetWorldTimerManager().SetTimer(RespawnTimer, RespawnTimerDelegate, 5.0f, false);

		// Feed
		if (ARWGameStateBase* RWGameState = GetGameState<ARWGameStateBase>())
		{
			RWGameState->OnKillFeedDelegate.Broadcast(Killer, Victim);
		}
	}
}

void ARWGameModeBase::RespawnCombatCharacter(ACombatCharacter* CharacterToRespawn)
{
	RestartPlayerAtPlayerStart(CharacterToRespawn->GetController(), ChoosePlayerStart(CharacterToRespawn->GetController()));
}
