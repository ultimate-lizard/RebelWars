#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "CombatAIController.generated.h"

class AFirearm;
class UInventoryComponent;

//UENUM(BlueprintType)
//enum class EAIActiveState : uint8
//{
//	AS_Idle				UMETA(DisplayName="Idle"),
//	AS_MeleeAttack		UMETA(DisplayName = "Melee Attack"),
//	AS_Shoot			UMETA(DisplayName = "Shoot")
//};

UENUM(BlueprintType)
enum class EAIPassiveState : uint8
{
	PS_None				UMETA(DisplayName = "None"),
	PS_Patrol			UMETA(DisplayName = "Patrol"),
	PS_Flee				UMETA(DisplayName = "Flee"),
	PS_TakeCover		UMETA(DisplayName = "Take Cover"),
	PS_Push				UMETA(DisplayName = "Push"),
	PS_KeepDistance		UMETA(DisplayName = "Keep Distance"),
	PS_TakeHight		UMETA(DisplayName = "Take Hight"),
	PS_MoveToTarget		UMETA(DisplayName = "Move To Target")
};

UCLASS()
class REBELWARS_API ACombatAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACombatAIController();

	//UFUNCTION(BlueprintPure)
	//bool IsCharacterArmed() const;

	//void SetNewTargetEnemy(APawn* InPawn);
	//APawn* GetTargetEnemy();

	//APawn* GetRememberedTargetEnemy();
	//void ForgetTargetEnemy();

	//APawn* EvaluateTargetEnemy() const;
	AActor* FindClosestEnemy() const;

	template <typename ActorClass>
	ActorClass* GetClosestSensedActor();

	void StartFireAt(AActor* InActor);
	void StopFire();

	// Amount of time in seconds until the AI Controller forgets the last seen target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory", Meta = (ClampMin = "0.0"))
	float TargetMemoryLength;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

	void Think();
	void UpdateInventoryManaging();
	void UpdateAttacking();

	bool IsAmmoLow(const UInventoryComponent& Inventory);
	void EquipBestWeapon(UInventoryComponent& Inventory);

	void SetTarget(AActor* NewTarget);
	void SetMovementTarget(AActor* NewTarget);
	//void UpdateActiveState(EAIActiveState NewState);
	void UpdatePassiveState(EAIPassiveState NewState);

protected:
	UPROPERTY()
	class UAISenseConfig_Sight* AIItemSightConfig;

	UInventoryComponent* PawnInventory;
	// bool bCanSeeAWeapon;

	// APawn* TargetEnemy;
	// APawn* RememberedTargetEnemy;
	// FTimerHandle MemoryTimer;

	bool bIsFiring;

	// FBlackboard::FKey IsArmedKey;

	// EAIActiveState CurrentActiveState;
	EAIPassiveState CurrentPassiveState;
	AActor* Target;
	AActor* MovementTarget;
};

template<typename ActorClass>
inline ActorClass* ACombatAIController::GetClosestSensedActor()
{
	TArray<ActorClass*> PerceptedTargets;
	if (UAIPerceptionComponent* AIPerception = GetPerceptionComponent())
	{
		for (auto Iter = AIPerception->GetPerceptualDataConstIterator(); Iter; ++Iter)
		{
			AActor* PerceptedActor = Iter.Key().ResolveObjectPtr();
			const FActorPerceptionInfo& PerceptionInfo = Iter.Value();

			if (PerceptedActor && PerceptionInfo.HasAnyKnownStimulus())
			{
				if (ActorClass* CastedPerceptedActor = Cast<ActorClass>(PerceptedActor))
				{
					PerceptedTargets.Add(CastedPerceptedActor);
				}
			}
		}
	}

	// Sort the targets to output the closest one
	FVector PawnLocation;

	if (APawn* AIPawn = GetPawn())
	{
		PawnLocation = AIPawn->GetActorLocation();
	}

	float MinDistance = TNumericLimits<float>::Max();
	ActorClass* ClosestActor = nullptr;
	for (ActorClass* PerceptedTarget : PerceptedTargets)
	{
		float TargetToPawnDistance = FVector::Distance(PerceptedTarget->GetActorLocation(), PawnLocation);
		if (TargetToPawnDistance < MinDistance)
		{
			MinDistance = TargetToPawnDistance;
			ClosestActor = PerceptedTarget;
		}
	}

	return ClosestActor;
}
