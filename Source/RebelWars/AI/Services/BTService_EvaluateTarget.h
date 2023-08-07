#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_EvaluateTarget.generated.h"

UCLASS()
class REBELWARS_API UBTService_EvaluateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_EvaluateTarget();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	struct FBlackboardKeySelector Target;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	struct FBlackboardKeySelector RememberedTarget;
};
