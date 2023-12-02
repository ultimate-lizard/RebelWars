#include "GameModes/RWGameModeBase.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Characters/CombatCharacter.h"
#include "GameModes/RWGameStateBase.h"
#include "Controllers/CombatAIController.h"
#include "Controllers/GameplayHumanController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RWPlayerStart.h"
#include "Player/RWPlayerState.h"
#include "EngineUtils.h"
#include "UI/RWHUD.h"

ARWGameModeBase::ARWGameModeBase() :
	Super()
{
	GameStateClass = ARWGameStateBase::StaticClass();
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

	if (ACombatCharacter* RestartedPawn = NewPlayer->GetPawn<ACombatCharacter>())
	{
		//if (!RestartedPawn->OnKillDelegate.Contains(this, FName(TEXT("OnCombatCharacterKilled"))))
		//{
		//	RestartedPawn->OnKillDelegate.AddDynamic(this, &ARWGameModeBase::OnCombatCharacterKilled);
		//}

		// RestartedPawn->Restart();
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

void ARWGameModeBase::InitGameState()
{
	Super::InitGameState();

	if (ARWGameStateBase* RWGameState = GetGameState<ARWGameStateBase>())
	{
		RWGameState->SetNumTeams(AvailableTeams.Num());
	}
}

void ARWGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ARWPlayerState* PlayerState = NewPlayer->GetPlayerState<ARWPlayerState>())
	{
		if (!RespawnTimers.Find(PlayerState->GetPlayerId()))
		{
			RespawnTimers.Add(PlayerState->GetPlayerId(), FTimerHandle());
		}

		// JoinTeam(PlayerState, EAffiliation::Spectators);
	}

	if (GetMatchState() != MatchState::InProgress)
	{
		// StartPlay();
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
	if (!NewPlayer || !StartSpot)
	{
		return;
	}

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

bool ARWGameModeBase::ReadyToStartMatch_Implementation()
{
	const uint32 MinPlayers = 3;
	if (NumPlayers < MinPlayers)
	{
		return false;
	}

	return Super::ReadyToStartMatch_Implementation();
}

bool ARWGameModeBase::MustSpectate_Implementation(APlayerController* NewPlayerController) const
{
	// TODO:
	return false;

	if (ARWPlayerState* PlayerState = NewPlayerController->GetPlayerState<ARWPlayerState>())
	{
		return PlayerState->GetTeam() == EAffiliation::None;
	}

	return true;
}

int32 ARWGameModeBase::GetBotDifficulty() const
{
	return BotDifficulty;
}

void ARWGameModeBase::JoinTeam(ARWPlayerState* PlayerState, EAffiliation Team)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *FString::Printf(TEXT("Player %s has joined team %i"), *PlayerState->GetPlayerName(), Team));

	APlayerController* PlayerStateController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (PlayerController->GetPlayerState<ARWPlayerState>() == PlayerState)
		{
			PlayerStateController = PlayerController;
		}
	}

	ACombatCharacter* PlayerPawn = PlayerState->GetPawn<ACombatCharacter>();

	if (PlayerPawn)
	{
		if (PlayerPawn->GetAffiliation() != Team)
		{
			// Kill
			PlayerPawn->SetHealth(0.0f);
			PlayerPawn->BroadcastBecomeRagdoll();
			PlayerStateController->ServerSetSpectatorLocation(PlayerPawn->GetActorLocation(), PlayerPawn->GetActorRotation());
		}
	}

	if (Team == EAffiliation::Spectators)
	{
		if (PlayerStateController)
		{
			PlayerStateController->ChangeState(NAME_Spectating);
			PlayerStateController->ClientGotoState(NAME_Spectating);
			if (FTimerHandle* RespawnTimer = RespawnTimers.Find(PlayerState->GetPlayerId()))
			{
				RespawnTimer->Invalidate();
				RespawnTimers.Remove(PlayerState->GetPlayerId());
			}
		}
	}
	else
	{
		if (!PlayerStateController->GetPawn())
		{
			RestartPlayer(PlayerStateController);
			PlayerPawn = PlayerState->GetPawn<ACombatCharacter>();
		}

		if (PlayerPawn)
		{
			PlayerPawn->SetAffiliation(Team);
		}
	}
}

bool ARWGameModeBase::IsTeamSelectAllowed() const
{
	return bAllowTeamSelect;
}

void ARWGameModeBase::StartMatch()
{
	Super::StartMatch();
}

void ARWGameModeBase::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();
}

void ARWGameModeBase::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// Spawn bots
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

AActor* ARWGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	// Choose a player start
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	ACombatCharacter* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<ACombatCharacter>() : nullptr;
	TArray<ARWPlayerStart*> UnOccupiedStartPoints;
	TArray<ARWPlayerStart*> OccupiedStartPoints;
	UWorld* World = GetWorld();

	for (TActorIterator<ARWPlayerStart> It(World); It; ++It)
	{
		ARWPlayerStart* PlayerStart = *It;

		if (PawnToFit->GetAffiliation() != PawnToFit->GetAffiliation())
		{
			continue;
		}

		FVector ActorLocation = PlayerStart->GetActorLocation();
		const FRotator ActorRotation = PlayerStart->GetActorRotation();
		if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
		{
			UnOccupiedStartPoints.Add(PlayerStart);
		}
		else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
		{
			OccupiedStartPoints.Add(PlayerStart);
		}
	}

	if (UnOccupiedStartPoints.Num() > 0)
	{
		FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
	}
	else if (OccupiedStartPoints.Num() > 0)
	{
		FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
	}

	return FoundPlayerStart;
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
