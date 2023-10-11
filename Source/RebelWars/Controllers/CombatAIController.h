#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"

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

	const TArray<TSoftObjectPtr<AFirearm>>& GetFirearmsInSight() const;
	const TArray<TSoftObjectPtr<APawn>>& GetPawnsInSight() const;

	void StartFireAt(AActor* InActor);
	void StopFire();

	bool CanSeeTarget(AActor* InActor) const;

public:
	UPROPERTY(EditDefaultsOnly)
	class UBehaviorTree* BehaviorTree;

	// Amount of time in seconds until the AI Controller forgets the last seen target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory", Meta = (ClampMin = "0.0"))
	float TargetMemoryLength;

	// TODO: TargetMemoryLengthDeviation

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	UPROPERTY()
	class UAISenseConfig_Sight* AIItemSightConfig;

	bool bCanSeeAWeapon;

	TArray<TSoftObjectPtr<AFirearm>> FirearmsInSight;
	TArray<TSoftObjectPtr<APawn>> PawnsInSight;

	APawn* TargetEnemy;
	APawn* RememberedTargetEnemy;
	FTimerHandle MemoryTimer;

	AActor* FireTarget;
	bool bIsFiring;
};
