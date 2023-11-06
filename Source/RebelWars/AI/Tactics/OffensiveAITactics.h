#pragma once

#include "CoreMinimal.h"
#include "AI/Tactics/AITacticsBase.h"

#include "OffensiveAITactics.generated.h"

UCLASS()
class REBELWARS_API UOffensiveAITactics : public UAITacticsBase
{
	GENERATED_BODY()
	
public:
	UOffensiveAITactics() {}
	UOffensiveAITactics(ACombatAIController* InAIController) : UAITacticsBase(InAIController) {}

	virtual bool CanExecute() const override;
	virtual void Execute() override;
};
