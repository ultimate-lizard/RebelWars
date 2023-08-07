#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"

#include "BTDecorator_IsArmed.generated.h"

UCLASS()
class REBELWARS_API UBTDecorator_IsArmed : public UBTDecorator
{
	GENERATED_BODY()
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
