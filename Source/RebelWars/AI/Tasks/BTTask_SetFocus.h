#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

#include "BTTask_SetFocus.generated.h"

UCLASS()
class REBELWARS_API UBTTask_SetFocus : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_SetFocus();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
};
