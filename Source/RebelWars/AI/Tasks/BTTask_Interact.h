#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

#include "BTTask_Interact.generated.h"

UCLASS()
class REBELWARS_API UBTTask_Interact : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_Interact();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
};
