#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_SetFireState.generated.h"

UCLASS()
class REBELWARS_API UBTTask_SetFireState : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_SetFireState();

protected:
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) {}
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

protected:
	UPROPERTY(EditAnywhere, Category = Blackboard)
	bool bIsFireActive;
};
