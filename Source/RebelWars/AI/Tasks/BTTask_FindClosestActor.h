#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

#include "BTTask_FindClosestActor.generated.h"

UCLASS()
class REBELWARS_API UBTTask_FindClosestActor : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_FindClosestActor();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

	UPROPERTY(EditAnywhere, Category = Blackboard)
	TSubclassOf<class AActor> ActorClass;
};
