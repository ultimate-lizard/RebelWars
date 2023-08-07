#include "AI/Services/BTService_EvaluateTarget.h"

#include "Controllers/CombatAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTService_EvaluateTarget::UBTService_EvaluateTarget()
{
	Target.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_EvaluateTarget, Target), AActor::StaticClass());
	RememberedTarget.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_EvaluateTarget, RememberedTarget), AActor::StaticClass());
}

void UBTService_EvaluateTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		Target.ResolveSelectedKey(*BBAsset);
		RememberedTarget.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_EvaluateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* TargetPawn = nullptr;
	APawn* RememberedTargetPawn = nullptr;

	if (ACombatAIController* CombatAIController = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
	{
		if (APawn* TargetEnemy = CombatAIController->GetTargetEnemy())
		{
			TargetPawn = TargetEnemy;
		}

		if (APawn* MemoryPawn = CombatAIController->GetRememberedTargetEnemy())
		{
			RememberedTargetPawn = MemoryPawn;
		}
	}

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValue<UBlackboardKeyType_Object>(Target.GetSelectedKeyID(), TargetPawn);
		BB->SetValue<UBlackboardKeyType_Object>(RememberedTarget.GetSelectedKeyID(), RememberedTargetPawn);
	}
}
