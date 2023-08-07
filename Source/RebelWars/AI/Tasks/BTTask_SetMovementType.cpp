#include "AI/Tasks/BTTask_SetMovementType.h"

#include "AIController.h"

EBTNodeResult::Type UBTTask_SetMovementType::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIOwner = OwnerComp.GetAIOwner())
	{
		if (ACombatCharacter* AICombatCharacter = Cast<ACombatCharacter>(AIOwner->GetCharacter()))
		{
			AICombatCharacter->SetMovementType(MovementType);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
