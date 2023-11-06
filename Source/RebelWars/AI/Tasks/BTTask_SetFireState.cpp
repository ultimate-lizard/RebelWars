#include "AI/Tasks/BTTask_SetFireState.h"

#include "Controllers/CombatAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTTask_SetFireState::UBTTask_SetFireState()
{
	bIsFireActive = false;

	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetFireState, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_SetFireState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (ACombatAIController* AIOwner = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
	{
		if (!bIsFireActive)
		{
			//AIOwner->StopFire();
			return EBTNodeResult::Succeeded;
		}

		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			if (AActor* TargetActor = Cast<AActor>(BB->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID())))
			{
				if (bIsFireActive)
				{
					//AIOwner->StartFireAt(TargetActor);
					return EBTNodeResult::Succeeded;
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}
