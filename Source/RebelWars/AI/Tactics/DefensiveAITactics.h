#pragma once

#include "CoreMinimal.h"
#include "AI/Tactics/AITacticsBase.h"

#include "DefensiveAITactics.generated.h"

UCLASS()
class REBELWARS_API UDefensiveAITactics : public UAITacticsBase
{
	GENERATED_BODY()
	
public:
	UDefensiveAITactics() {}
	UDefensiveAITactics(ACombatAIController* InAIController) : UAITacticsBase(InAIController) {}

	virtual bool CanExecute() const override;
	virtual void Execute() override;
};
