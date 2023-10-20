#include "AI/Tasks/BTTask_FindClosestItem.h"

#include "Controllers/CombatAIController.h"
#include "Items/Firearm.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTTask_FindClosestItem::UBTTask_FindClosestItem()
{
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindClosestItem, BlackboardKey), AFirearm::StaticClass());
}

EBTNodeResult::Type UBTTask_FindClosestItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (ACombatAIController* AIOwner = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
		{
			/*const TArray<TSoftObjectPtr<AFirearm>>& Firearms = AIOwner->GetFirearmsInSight();
			
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
					BB->SetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID(), ClosestActor.Get());
					return EBTNodeResult::Succeeded;
				}
			}*/
		}
	}

	return EBTNodeResult::Failed;
}
