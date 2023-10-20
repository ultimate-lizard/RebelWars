#include "AI/Tasks/BTTask_FindClosestActor.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Perception/AIPerceptionComponent.h"

#include "Controllers/CombatAIController.h"

UBTTask_FindClosestActor::UBTTask_FindClosestActor()
{
    BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindClosestActor, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FindClosestActor::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (ACombatAIController* OwnerAIController = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
    {
        // Get all visible targets
        TArray<AActor*> PerceptedTargets;
        if (UAIPerceptionComponent* AIPerception = OwnerAIController->GetPerceptionComponent())
        {
            for (auto Iter = AIPerception->GetPerceptualDataConstIterator(); Iter; ++Iter)
            {
                AActor* PerceptedActor = Iter.Key().ResolveObjectPtr();
                const FActorPerceptionInfo& PerceptionInfo = Iter.Value();

                if (PerceptedActor)
                {
                    if (PerceptedActor->IsA(ActorClass) && PerceptionInfo.HasAnyCurrentStimulus())
                    {
                        PerceptedTargets.Add(PerceptedActor);
                    }
                }
            }
        }

        // Sort the targets to output the closest one
        FVector PawnLocation;

        if (APawn* AIPawn = OwnerAIController->GetPawn())
        {
            PawnLocation = AIPawn->GetActorLocation();
        }

        float MinDistance = TNumericLimits<float>::Max();
        AActor* ClosestActor = nullptr;
        for (AActor* Target : PerceptedTargets)
        {
            float TargetToPawnDistance = FVector::Distance(Target->GetActorLocation(), PawnLocation);
            if (TargetToPawnDistance < MinDistance)
            {
                MinDistance = TargetToPawnDistance;
                ClosestActor = Target;
            }
        }

        if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
        {
            BB->SetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID(), ClosestActor);
            return EBTNodeResult::Succeeded;
        }
    }

    return EBTNodeResult::Failed;
}
