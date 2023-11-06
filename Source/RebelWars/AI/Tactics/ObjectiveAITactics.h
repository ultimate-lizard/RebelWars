#pragma once

#include "CoreMinimal.h"
#include "AI/Tactics/AITacticsBase.h"

#include "ObjectiveAITactics.generated.h"

UCLASS()
class REBELWARS_API UObjectiveAITactics : public UAITacticsBase
{
	GENERATED_BODY()
	
public:
	UObjectiveAITactics() {}
	UObjectiveAITactics(ACombatAIController* InAIController) : UAITacticsBase(InAIController) {}

	virtual bool CanExecute() const override;
	virtual void Execute() override;
};
