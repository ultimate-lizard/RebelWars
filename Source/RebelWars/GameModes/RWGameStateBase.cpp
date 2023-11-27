#include "GameModes/RWGameStateBase.h"

ARWGameStateBase::ARWGameStateBase()
	: Super()
{
	NumTeams = 0;
}

void ARWGameStateBase::SetNumTeams(int32 InNumTeams)
{
	NumTeams = InNumTeams;
}

int32 ARWGameStateBase::GetNumTeams() const
{
	return NumTeams;
}
