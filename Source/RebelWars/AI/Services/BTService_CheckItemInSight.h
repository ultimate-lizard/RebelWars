#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"

#include "BTService_CheckItemInSight.generated.h"

UCLASS()
class REBELWARS_API UBTService_CheckItemInSight : public UBTService_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTService_CheckItemInSight();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	TSubclassOf<class AItemBase> ItemClass;
};
