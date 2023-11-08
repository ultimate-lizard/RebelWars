#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AICombatTechniqueComponent.generated.h"

class ACombatAIController;
class ACombatCharacter;
class AFirearm;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REBELWARS_API UAICombatTechniqueComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAICombatTechniqueComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FVector CalcFocalPointAccuracyModifier();

	void TickShootingTechnique();

	void InitCombatSkills();

	// The unmodified starting amount of aim adjustments the AI will do until full precision
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", Meta = (ClampMin = "1.0"))
	float AimAttempFrequencyBase;

	// The amount of aim adjustments decreased from base amount per skill until full precision
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", Meta = (ClampMin = "1.0"))
	float AimAttempFrequencyPerSkill;

	// The starting base (and worst) aim drawback for AI aiming
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", Meta = (ClampMin = "0.0"))
	float AimAccuracyBase;

	// The amount by which the AI aim will get better per aiming attempt
	UPROPERTY(EditDefaultsOnly, Category = "Aiming", Meta = (ClampMin = "0.0"))
	float AimAccuracyPerSkill;

	// The minimum possible amount of shots in a burst
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	int32 MinBurst = 3;

	// The frequency of bursts in milliseconds
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	float AutoFireFrequencyBase = 300.0f;

	// The frequency of semi automatic shots in milliseconds
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	float SemiAutoFireFrequencyBase = 200.0f;

	// The amount of time added to the base frequency per skill in milliseconds
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	float BaseAutoFireFrequencyPerSkill = 50.0f;

	// The amount of time added to the base frequency per skill in milliseconds
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	float BaseSemiAutoFireFrequencyPerSkill = 25.0f;

	// Which level of aggression is required to do full auto shooting
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	int32 FullAutoAggressionRequirement = 8;

	// What is the maximum skill required to do full auto shooting. Useful for very low skill bots
	UPROPERTY(EditDefaultsonly, Category = "Shooting", Meta = (ClampMin = "1"))
	int32 FullAutoMaxSkillRequirement = 2;

protected:
	void TickFullAuto(ACombatCharacter* InCombatPawn, AFirearm* InFirearm);
	void TickSemiAuto(ACombatCharacter* InCombatPawn, AFirearm* InFirearm);

	UPROPERTY(Transient)
	ACombatAIController* AIOwner;

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

	float LastFireTime;
	int32 BurstStartAmmo;
	int32 BurstShotsMade;

	// These members are assigned durign runtime based on the AI's skill and aggression
	int32 BurstShotsAmount;
	float AutoFireFrequency;
	float SemiAutoFireFrequency;
	bool bFullAuto;
};
