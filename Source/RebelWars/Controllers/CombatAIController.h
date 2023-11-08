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
class UAICombatTechniqueComponent;
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

	// This reaction response will be specific to this optional weapon class
	UPROPERTY(EditAnywhere)
	TSubclassOf<AFirearm> WeaponOverrideClass;
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
	bool IsFiring() const;

	void React(EReaction InReaction);

	AFirearm* GetPawnWeapon() const;

	FRotator GetTargetControlRotation() const;
	void SetTargetControlRotation(FRotator InRotation);

	UPROPERTY(EditDefaultsOnly)
	class UBehaviorTree* BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FReactionInfo> Reactions;

	UFUNCTION(BlueprintPure)
	int32 GetSkill() const;

	UFUNCTION(BlueprintPure)
	int32 GetAggression() const;

	UFUNCTION(BlueprintPure)
	int32 GetMaxSkill() const;

	UFUNCTION(BlueprintPure)
	int32 GetMaxAggression() const;

	AFirearm* GetEquippedFirearm() const;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	virtual FVector GetFocalPointOnActor(const AActor* Actor) const override;

protected:
	UPROPERTY(EditDefaultsOnly)
	UAICombatTechniqueComponent* AICombatTechniqueComponent;

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
