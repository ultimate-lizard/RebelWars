#include "AI/Tasks/BTTask_Interact.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Controllers/CombatAIController.h"
#include "Components/InteractableComponent.h"

UBTTask_Interact::UBTTask_Interact()
{
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_Interact, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_Interact::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (ACombatAIController* AIOwner = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
		{
			UBlackboardKeyType_Object::FDataType AAA = BB->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
			if (AActor* InteractableActor = Cast<AActor>(BB->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID())))
			{
				if (UInteractableComponent* Interactable = InteractableActor->FindComponentByClass<UInteractableComponent>())
				{
					Interactable->Interact(AIOwner->GetPawn());

					return EBTNodeResult::Succeeded;
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}
