#include "AI/Services/BTService_CheckItemInSight.h"

#include "Controllers/CombatAIController.h"
#include "Items/Firearm.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"

UBTService_CheckItemInSight::UBTService_CheckItemInSight()
{
	BlackboardKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CheckItemInSight, BlackboardKey));
}

void UBTService_CheckItemInSight::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	bool bItemInSight = false;

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (ACombatAIController* OwnerAIController = Cast<ACombatAIController>(OwnerComp.GetAIOwner()))
		{
			const TArray<TSoftObjectPtr<AFirearm>>& Firearms = OwnerAIController->GetFirearmsInSight();
			for (const TSoftObjectPtr<AFirearm>& FirearmPtr : Firearms)
			{
				if (const AFirearm* Firearm = FirearmPtr.Get())
				{
					const UClass* FirearmClass = Firearm->GetClass();
					if (FirearmClass == ItemClass || FirearmClass->IsChildOf(ItemClass))
					{
						bItemInSight = true;
					}
				}
			}
		}

		BB->SetValue<UBlackboardKeyType_Bool>(BlackboardKey.GetSelectedKeyID(), bItemInSight);
	}
}
