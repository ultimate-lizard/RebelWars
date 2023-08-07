#include "AI/Tasks/BTTask_FindRandomLocation.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"
#include "AIController.h"

UBTTask_FindRandomLocation::UBTTask_FindRandomLocation()
{
    BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindRandomLocation, BlackboardKey));
}

EBTNodeResult::Type UBTTask_FindRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
    {
        if (AAIController* AIOwner = OwnerComp.GetAIOwner())
        {
            if (APawn* AIPawn = AIOwner->GetPawn())
            {
                FVector RandomLocation;
                UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), AIPawn->GetActorLocation(), RandomLocation, 1500.0f);
                BB->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), RandomLocation);
                return EBTNodeResult::Succeeded;
            }
        }
    }

    return EBTNodeResult::Failed;
}
