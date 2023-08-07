#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Utils/TeamStatics.h"

#include "RWPlayerStart.generated.h"

UCLASS()
class REBELWARS_API ARWPlayerStart : public APlayerStart
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Object)
	EAffiliation PlayerStartAffiliation;
};
