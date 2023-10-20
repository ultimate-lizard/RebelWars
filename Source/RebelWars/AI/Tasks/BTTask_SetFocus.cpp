#include "AI/Tasks/BTTask_SetFocus.h"

#include "Controllers/CombatAIController.h"

UBTTask_SetFocus::UBTTask_SetFocus()
{
    BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetFocus, BlackboardKey), AActor::StaticClass());
    // BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetFocus, BlackboardKey));
}

EBTNodeResult::Type UBTTask_SetFocus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (ACombatAIController* AIController = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
    {
        if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
        {
            if (AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(BlackboardKey.SelectedKeyName)))
            {
                AIController->SetFocus(TargetActor);
            }
            else
            {
                AIController->ClearFocus(EAIFocusPriority::Gameplay);
            }

            return EBTNodeResult::Succeeded;
        }
    }

    return EBTNodeResult::Failed;
}
