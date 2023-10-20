#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"

#include "BTService_EvaluateEnemyTarget.generated.h"

UCLASS()
class REBELWARS_API UBTService_EvaluateEnemyTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_EvaluateEnemyTarget();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// Outputs the closest visible target or the last visible target if no other visible targets exist
	UPROPERTY(EditAnywhere, Category = Blackboard)
	struct FBlackboardKeySelector Target;

	// Outputs whether the AI has line of sight to Target
	UPROPERTY(EditAnywhere, Category = Blackboard)
	struct FBlackboardKeySelector HasLOS;
};
