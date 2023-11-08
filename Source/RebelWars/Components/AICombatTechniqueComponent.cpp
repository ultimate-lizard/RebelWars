#include "Components/AICombatTechniqueComponent.h"

#include "Controllers/CombatAIController.h"
#include "Characters/CombatCharacter.h"
#include "Items/Firearm.h"

UAICombatTechniqueComponent::UAICombatTechniqueComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	AimAttempFrequencyBase = 1100.0f;
	AimAttempFrequencyPerSkill = 100.0f;
	AimAccuracyBase = 200.0f;
	AimAccuracyPerSkill = 20.0f;

	const float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());

	MaxAimAttemps = 5;
	AimAttemptsLeft = MaxAimAttemps;
	AimAttemptFrequency = 500.0f;
	LastAimAttemptTime = CurrentTime;
	LastAimModifier = FVector::ZeroVector;
	AimAccuracyStartingPoint = 200.0f;

	LastFireTime = CurrentTime;
	BurstStartAmmo = 0;
	BurstShotsMade = 0;

	BurstShotsAmount = 3;
	AutoFireFrequency = 300.0f;
	SemiAutoFireFrequency = 300.0f;
	bFullAuto = false;
}

void UAICombatTechniqueComponent::BeginPlay()
{
	Super::BeginPlay();

	AIOwner = Cast<ACombatAIController>(GetOwner());
}

// Called every frame
void UAICombatTechniqueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	int32 Skill = AIOwner->GetSkill();

	MaxAimAttemps = AIOwner->GetMaxSkill() - Skill;
	AimAttemptFrequency = AimAttempFrequencyBase - Skill * AimAttempFrequencyPerSkill;
	AimAccuracyStartingPoint = AimAccuracyBase - AimAccuracyPerSkill * Skill;
}

FVector UAICombatTechniqueComponent::CalcFocalPointAccuracyModifier()
{
	if (!AIOwner)
	{
		return FVector::ZeroVector;
	}

	const float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	if (CurrentTime - LastAimAttemptTime >= AimAttemptFrequency && AIOwner->IsAimedAtTarget())
	{
		AimAttemptsLeft = FMath::Clamp(AimAttemptsLeft, 0, MaxAimAttemps);
		LastAimAttemptTime = CurrentTime;
		LastAimModifier = FMath::VRand() * ((AimAccuracyStartingPoint / MaxAimAttemps) * AimAttemptsLeft);

		if (!LastAimModifier.Size())
		{
			LastAimModifier = FVector::ZeroVector;
		}

		AimAttemptsLeft--;
	}

	if (!AIOwner->IsAimedAtTarget())
	{
		AimAttemptsLeft = MaxAimAttemps;
	}

	return LastAimModifier;
}

static float LastFireTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());

void UAICombatTechniqueComponent::TickShootingTechnique()
{
	if (!AIOwner)
	{
		return;
	}

	ACombatCharacter* CombatPawn = AIOwner->GetPawn<ACombatCharacter>();
	if (!CombatPawn)
	{
		return;
	}

	AFirearm* PrimaryFirearm = AIOwner->GetEquippedFirearm();
	if (!PrimaryFirearm)
	{
		return;
	}

	// Reset burst
	//if (PrimaryFirearm->IsReloading())
	//{
	//	BurstShotsMade = 0;
	//}

	if (AIOwner->IsFiring())
	{
		switch (PrimaryFirearm->CurrentFireMode)
		{
		case EFirearmFireMode::Auto:
			TickFullAuto(CombatPawn, PrimaryFirearm);
			break;
		case EFirearmFireMode::SemiAuto:
			TickSemiAuto(CombatPawn, PrimaryFirearm);
			break;
		}

		if (PrimaryFirearm->CurrentMagAmmo <= 0)
		{
			CombatPawn->StopPrimaryFire();
			CombatPawn->Reload();
			BurstShotsMade = 0;
		}
	}
	else
	{
		CombatPawn->StopPrimaryFire();
	}

	// Reload when no targets
	if (!AIOwner->GetTarget())
	{
		CombatPawn->Reload();
	}
}

void UAICombatTechniqueComponent::InitCombatSkills()
{
	if (!AIOwner)
	{
		return;
	}

	const int32 Skill = AIOwner->GetSkill();
	const int32 Aggression = AIOwner->GetAggression();

	bFullAuto = false;
	BurstShotsAmount = AIOwner->GetMaxSkill() - Skill + FMath::RandRange(1, MinBurst);
	AutoFireFrequency = AutoFireFrequencyBase + (AIOwner->GetMaxSkill() - Skill) * BaseAutoFireFrequencyPerSkill;
	SemiAutoFireFrequency = SemiAutoFireFrequencyBase + (AIOwner->GetMaxSkill() - Skill) * BaseSemiAutoFireFrequencyPerSkill;

	if (Aggression >= FullAutoAggressionRequirement || Skill <= FullAutoMaxSkillRequirement)
	{
		bFullAuto = true;
		SemiAutoFireFrequency = 50.0f;
	}
}

void UAICombatTechniqueComponent::TickFullAuto(ACombatCharacter* InCombatPawn, AFirearm* InFirearm)
{
	float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	if (CurrentTime - LastFireTime >= AutoFireFrequency || bFullAuto)
	{
		if (!InFirearm->IsReloading() && !InFirearm->IsFiring())
		{
			BurstStartAmmo = InFirearm->CurrentMagAmmo;
			InCombatPawn->StartPrimaryFire();
		}
	}

	BurstShotsMade = BurstStartAmmo - InFirearm->CurrentMagAmmo;

	if (BurstShotsMade >= BurstShotsAmount)
	{
		InCombatPawn->StopPrimaryFire();
		LastFireTime = CurrentTime;
		BurstShotsMade = 0;
		BurstStartAmmo = InFirearm->CurrentMagAmmo;
	}
}

void UAICombatTechniqueComponent::TickSemiAuto(ACombatCharacter* InCombatPawn, AFirearm* InFirearm)
{
	if (!InFirearm->IsReloading())
	{
		float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
		if (CurrentTime - LastFireTime >= SemiAutoFireFrequency)
		{
			InCombatPawn->StopPrimaryFire();
			InCombatPawn->StartPrimaryFire();
			LastFireTime = CurrentTime;
		}
	}
}
