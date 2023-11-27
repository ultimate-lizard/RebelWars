#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericTeamAgentInterface.h"

#include "TeamStatics.generated.h"

UENUM(BlueprintType)
enum class EAffiliation : uint8
{
	None = 0,
	Rebels = 1,
	CRF = 2
};

USTRUCT()
struct FTeam
{
	GENERATED_BODY()

	EAffiliation Affiliation;
	TArray<APlayerState*> Players;
};

UCLASS()
class REBELWARS_API UTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION()
	static ETeamAttitude::Type SolveAttitudeImpl(FGenericTeamId TeamA, FGenericTeamId TeamB);

	static uint8 GetTeamIdFromAffiliation(EAffiliation InAffiliation);
	static EAffiliation GetTeamAffiliationFromTeamId(uint8 InTeamId);
};
