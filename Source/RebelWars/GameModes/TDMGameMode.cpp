#include "GameModes/TDMGameMode.h"

ATDMGameMode::ATDMGameMode() 
	: Super()
{
	AvailableTeams.Add(EAffiliation::Rebels);
	AvailableTeams.Add(EAffiliation::CRF);
}
