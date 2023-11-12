#include "GameModes/RWGameModeBase.h"

#include "GameFramework/PlayerState.h"
#include "Characters/CombatCharacter.h"
#include "GameModes/RWGameStateBase.h"
#include "Controllers/CombatAIController.h"
#include "Kismet/GameplayStatics.h"

ARWGameModeBase::ARWGameModeBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
}

APawn* ARWGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	APawn* SpawnedPawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);

	BotsSpawned = 0;

	return SpawnedPawn;
}

void ARWGameModeBase::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer && NewPlayer->StartSpot == nullptr)
	{
		NewPlayer->StartSpot = FindPlayerStart(NewPlayer);
	}

	Super::RestartPlayer(NewPlayer);

	if (APawn* RestartedPawn = NewPlayer->GetPawn())
	{
		if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(RestartedPawn))
		{
			if (!CombatCharacter->OnKillDelegate.Contains(this, FName(TEXT("OnCombatCharacterKilled"))))
			{
				CombatCharacter->OnKillDelegate.AddDynamic(this, &ARWGameModeBase::OnCombatCharacterKilled);
			}
		}
	}
}

void ARWGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	bBotsEnabled = UGameplayStatics::GetIntOption(Options, TEXT("bBotsEnabled"), 0) != 0;
	BotCount = UGameplayStatics::GetIntOption(Options, TEXT("BotCount"), 0);
	BotDifficulty = UGameplayStatics::GetIntOption(Options, TEXT("BotDifficulty"), 0);

	if (bBotsEnabled)
	{
		NumBots = BotCount;
	}
}

void ARWGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>())
	{
		if (!RespawnTimers.Find(PlayerState->GetPlayerId()))
		{
			RespawnTimers.Add(PlayerState->GetPlayerId(), FTimerHandle());
		}
	}
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

int32 ARWGameModeBase::GetBotDifficulty() const
{
	return BotDifficulty;
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
				for (int32 i = 0; i < NumBots; ++i)
				{
					if (ACombatAIController* SpawnedAIController = Cast<ACombatAIController>(World->SpawnActor(DefaultAIControllerObject->GetClass())))
					{
						SpawnedAIController->SetDifficulty(static_cast<EBotDifficulty>(BotDifficulty));

						if (APlayerState* PlayerState = SpawnedAIController->GetPlayerState<APlayerState>())
						{
							--BotsSpawned;
							PlayerState->SetPlayerId(BotsSpawned);
							PlayerState->SetPlayerName(FString::Printf(TEXT("Bot %i"), FMath::Abs(BotsSpawned)));
						}
					}
				}
			}
		}
	}
}

void ARWGameModeBase::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	for (auto Iter = RespawnTimers.CreateIterator(); Iter; ++Iter)
	{
		GetWorldTimerManager().ClearTimer(Iter.Value());
	}

	RespawnTimers.Empty();

	for (FConstControllerIterator Iter = GetWorld()->GetControllerIterator(); Iter; ++Iter)
	{
		if (ACombatAIController* AI = Cast<ACombatAIController>(*Iter))
		{
			RestartPlayer(AI);
		}

		if (AController* Controller = Iter->Get())
		{
			if (APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>())
			{
				if (!RespawnTimers.Find(PlayerState->GetPlayerId()))
				{
					RespawnTimers.Add(PlayerState->GetPlayerId(), FTimerHandle());
				}
			}
		}
	}
}

void ARWGameModeBase::OnCombatCharacterKilled(AActor* Killer, ACombatCharacter* Victim)
{
	if (APlayerState* KilledPlayer = Victim->GetPlayerState())
	{
		const FString CharName = KilledPlayer->GetPlayerName() + TEXT(" has been killed");
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, *CharName);

		if (FTimerHandle* RespawnTimer = RespawnTimers.Find(KilledPlayer->GetPlayerId()))
		{
			if (GetWorldTimerManager().IsTimerActive(*RespawnTimer))
			{
				GetWorldTimerManager().ClearTimer(*RespawnTimer);
			}

			FTimerDelegate RespawnTimerDelegate = FTimerDelegate::CreateUObject(this, &ARWGameModeBase::RespawnCombatCharacter, Victim);
			GetWorldTimerManager().SetTimer(*RespawnTimer, RespawnTimerDelegate, 5.0f, false);
		}

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
