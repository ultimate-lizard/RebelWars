#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Utils/TeamStatics.h"

#include "RWPlayerState.generated.h"

UCLASS()
class REBELWARS_API ARWPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	void SetTeam(EAffiliation Team);
	EAffiliation GetTeam() const;

private:
	EAffiliation Team;
};
