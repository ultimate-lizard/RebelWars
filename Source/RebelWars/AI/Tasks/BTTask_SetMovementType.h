#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "Characters/CombatCharacter.h"

#include "BTTask_SetMovementType.generated.h"

UCLASS()
class REBELWARS_API UBTTask_SetMovementType : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) {}
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

protected:
	UPROPERTY(EditAnywhere, Category = Blackboard)
	ECharacterMovementType MovementType;
};
