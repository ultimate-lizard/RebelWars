#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "CombatAIController.generated.h"

class AFirearm;

UCLASS()
class REBELWARS_API ACombatAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACombatAIController();

	UFUNCTION(BlueprintPure)
	bool IsCharacterArmed() const;

	void SetNewTargetEnemy(APawn* InPawn);
	APawn* GetTargetEnemy();

	APawn* GetRememberedTargetEnemy();
	void ForgetTargetEnemy();

	APawn* EvaluateTargetEnemy() const;
	AActor* FindClosestEnemy() const;

	void StartFireAt(AActor* InActor);
	void StopFire();

public:
	UPROPERTY(EditDefaultsOnly)
	class UBehaviorTree* BehaviorTree;

	// Amount of time in seconds until the AI Controller forgets the last seen target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory", Meta = (ClampMin = "0.0"))
	float TargetMemoryLength;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

protected:
	UPROPERTY()
	class UAISenseConfig_Sight* AIItemSightConfig;

	bool bCanSeeAWeapon;

	APawn* TargetEnemy;
	APawn* RememberedTargetEnemy;
	FTimerHandle MemoryTimer;

	AActor* FireTarget;
	bool bIsFiring;

	FBlackboard::FKey IsArmedKey;
};
