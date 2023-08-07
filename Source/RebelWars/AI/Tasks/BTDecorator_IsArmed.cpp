#include "AI/Tasks/BTDecorator_IsArmed.h"

#include "Controllers/CombatAIController.h"

bool UBTDecorator_IsArmed::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (ACombatAIController* AIOwner = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
	{
		return AIOwner->IsCharacterArmed();
	}

	return false;
}
