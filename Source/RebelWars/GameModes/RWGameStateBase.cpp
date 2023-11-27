#include "GameModes/RWGameStateBase.h"

#include "Net/UnrealNetwork.h"
#include "Player/RWPlayerState.h"
#include "Characters/CombatCharacter.h"

ARWGameStateBase::ARWGameStateBase()
	: Super()
{
	NumTeams = 3;
}

void ARWGameStateBase::AddPlayerToTeam(ARWPlayerState* InPlayerState, EAffiliation InTeam)
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		return;
	}

	//TArray<APlayerState*> PlayersInTeam;

	//// Gather players of team:
	//for (auto Iter = PlayerArray.CreateConstIterator(); Iter; ++Iter)
	//{
	//	if (APlayerState* PlayerState = *Iter)
	//	{
	//		if (ACombatCharacter* PlayerPawn = PlayerState->GetPawn<ACombatCharacter>())
	//		{
	//			if (PlayerPawn->Affiliation == InTeam)
	//			{
	//				PlayersInTeam.Add(PlayerState);
	//			}
	//		}
	//	}
	//}

	if (InPlayerState)
	{
		if (ACombatCharacter* PlayerPawn = InPlayerState->GetPawn<ACombatCharacter>())
		{
			PlayerPawn->Affiliation = InTeam;
		}
	}
}

void ARWGameStateBase::SetNumTeams(int32 InNumTeams)
{
	NumTeams = InNumTeams;
}

int32 ARWGameStateBase::GetNumTeams() const
{
	return NumTeams;
}

void ARWGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARWGameStateBase, NumTeams);
}
