#include "AI/Tasks/BTTask_FindClosestItem.h"

#include "Controllers/CombatAIController.h"
#include "Items/Firearm.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTTask_FindClosestItem::UBTTask_FindClosestItem()
{
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindClosestItem, BlackboardKey));
}

EBTNodeResult::Type UBTTask_FindClosestItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (ACombatAIController* AIOwner = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
		{
			const TArray<TSoftObjectPtr<AFirearm>>& Firearms = AIOwner->GetFirearmsInSight();
			
			TSoftObjectPtr<AFirearm> ClosestActor;
			float ClosestDistance = TNumericLimits<float>::Max();
			if (APawn* AIPawn = AIOwner->GetPawn())
			{
				for (const TSoftObjectPtr<AFirearm>& FirearmPtr : Firearms)
				{
					if (AFirearm* Firearm = FirearmPtr.Get())
					{
						float Distance = FVector::Distance(AIPawn->GetActorLocation(), Firearm->GetActorLocation());
						if (Distance < ClosestDistance)
						{
							ClosestActor = FirearmPtr;
							ClosestDistance = Distance;
						}
					}
				}
			}

			if (ClosestActor.Get())
			{
				if (ClosestActor.Get()->GetClass() == ItemClass || ClosestActor.Get()->GetClass()->IsChildOf(ItemClass))
				{
					BB->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), ClosestActor->GetActorLocation());
					return EBTNodeResult::Succeeded;
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}
