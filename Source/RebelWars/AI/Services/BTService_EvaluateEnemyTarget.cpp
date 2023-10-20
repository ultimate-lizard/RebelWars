#include "AI/Services/BTService_EvaluateEnemyTarget.h"

#include "Controllers/CombatAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Perception/AIPerceptionComponent.h"

/*
	1. Finds closest enemy target
	2. If lost sight to target - look for new
	3. If no other targets found, chase the previous target

	Use parameters from CombatAIController to tweak difficulty etc.
*/

UBTService_EvaluateEnemyTarget::UBTService_EvaluateEnemyTarget()
{
	Target.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_EvaluateEnemyTarget, Target), AActor::StaticClass());
	HasLOS.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_EvaluateEnemyTarget, Target));
}

void UBTService_EvaluateEnemyTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		Target.ResolveSelectedKey(*BBAsset);
		HasLOS.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_EvaluateEnemyTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (ACombatAIController* CombatAIController = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			AActor* NextTarget = nullptr;
			bool bHasLOS = false;

			// Has some visible target
			if (AActor* ClosestEnemy = CombatAIController->FindClosestEnemy())
			{
				NextTarget = ClosestEnemy;
				bHasLOS = true;
			}
			// Has no visible targets. Target previous if not aged
			else
			{
				if (AActor* LastTarget = Cast<AActor>(BB->GetValueAsObject(Target.SelectedKeyName)))
				{
					if (UAIPerceptionComponent* AIPerception = CombatAIController->GetPerceptionComponent())
					{
						if (const FActorPerceptionInfo* PerceptionInfo = AIPerception->GetActorInfo(*LastTarget))
						{
							if (PerceptionInfo->HasAnyKnownStimulus())
							{
								NextTarget = LastTarget;

								if (PerceptionInfo->HasAnyCurrentStimulus())
								{
									bHasLOS = true;
								}
							}
						}
					}
				}
			}

			BB->SetValueAsObject(Target.SelectedKeyName, NextTarget);
			BB->SetValueAsBool(HasLOS.SelectedKeyName, bHasLOS);
		}
	}
}
