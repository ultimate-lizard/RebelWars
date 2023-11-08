#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "AI/Tactics/AITacticsBase.h"

#include "CombatAIController.generated.h"

class AFirearm;
class UInventoryComponent;
class UAITacticsBase;

UENUM(BlueprintType)
enum class EAIPassiveState : uint8
{
	PS_None				UMETA(DisplayName = "None"),
	PS_Patrol			UMETA(DisplayName = "Patrol"),
	PS_TakeCover		UMETA(DisplayName = "Take Cover"),
	PS_Push				UMETA(DisplayName = "Push"),
	PS_KeepDistance		UMETA(DisplayName = "Keep Distance"),
	PS_TakeHight		UMETA(DisplayName = "Take Hight"),
	PS_MoveToTarget		UMETA(DisplayName = "Move To Target")
};

UENUM()
enum class EBotDifficulty : uint8
{
	Difficulty_Easy,
	Difficulty_Medium,
	Difficulty_Hard
};

USTRUCT()
struct FReactionResponse
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EAIPassiveState MovementBehavior;

	UPROPERTY(EditAnywhere)
	int32 SkillRequired = 0;

	UPROPERTY(EditAnywhere)
	int32 AggressionRequired = 0;
};

USTRUCT(BlueprintType)
struct FReactionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EReaction Reaction;

	UPROPERTY(EditAnywhere)
	TArray<FReactionResponse> Responses;
};

UCLASS()
class REBELWARS_API ACombatAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACombatAIController();

	FVector CalcFocalPointAccuracyModifier();

	bool IsAimedAtTarget() const;

	void InitDifficulty(EBotDifficulty InDifficulty);

	AActor* FindClosestEnemy() const;

	template <typename ActorClass>
	ActorClass* FindClosestSensedActor();

	bool IsArmed() const;

	void SetFiringEnabled(bool bEnabled);

	void SetTarget(AActor* NewTarget);
	AActor* GetTarget() const;

	AActor* GetMovementTarget() const;

	bool IsAmmoLow() const;

	UInventoryComponent* GetPawnInventory() const;

	void SetMovementBehavior(EAIPassiveState NewState);
	void SetMovementTarget(AActor* NewTarget);

	void EquipBestWeapon();

	bool IsReloading() const;

	void React(EReaction InReaction);

	AFirearm* GetPawnWeapon() const;

	UPROPERTY(EditDefaultsOnly)
	class UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FReactionInfo> Reactions;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

	void TickShootingTechnique();

protected:
	UPROPERTY()
	class UAISenseConfig_Sight* AIItemSightConfig;

	UInventoryComponent* PawnInventory;

	bool bIsFiring;

	EAIPassiveState MovementBehavior;
	AActor* Target;
	AActor* MovementTarget;
	// Rotation target for smooth AI rotating
	FRotator TargetControlRotation;

	UPROPERTY()
	TArray<UAITacticsBase*> Tactics;

	// The higher the skill - the faster the AI thinks and the better decisions it makes. Max 10
	UPROPERTY()
	int32 Skill;

	// How aggressive the AI decisions are. Max 10
	UPROPERTY()
	int32 Aggression;

	FTimerHandle ReactionTimer;

	// How frequent the AI makes decisions. Counted in seconds. Calculates based on Skill
	float ReactionTime;

	// How many times the AI tries to adjust the aim until it reaches max accuracy
	int32 MaxAimAttemps;
	int32 AimAttemptsLeft;

	// How frequent should the AI adjust its aim in milliseconds
	float AimAttemptFrequency;

	// The time stamp in milliseconds when the last aim attempt happened
	float LastAimAttemptTime;

	// The last used aim modifier. Should be remembered to prevent making aim modifiers every tick
	FVector LastAimModifier;

	// The starting point for the AI aiming. It gets better every Aim Attempt
	float AimAccuracyStartingPoint;

	int32 BurstStartAmmo = 0;
	int32 BurstShotsMade = 0;
};

template<typename ActorClass>
inline ActorClass* ACombatAIController::FindClosestSensedActor()
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
