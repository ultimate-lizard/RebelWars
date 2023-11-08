#include "Controllers/CombatAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionComponent.h"
#include "Items/Firearm.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Characters/CombatCharacter.h"
#include "AI/Tactics/OffensiveAITactics.h"
#include "AI/Tactics/DefensiveAITactics.h"
#include "AI/Tactics/ObjectiveAITactics.h"
#include "DrawDebugHelpers.h"

ACombatAIController::ACombatAIController()
{
	bWantsPlayerState = true;

	Blackboard = CreateDefaultSubobject<UBlackboardComponent>(FName(TEXT("Blackboard")));
	BrainComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(FName(TEXT("Behavior Tree")));
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(FName(TEXT("AI Perception")));
	check(PerceptionComponent);

	AIItemSightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(FName(TEXT("AI Sight Config")));
	check(AIItemSightConfig);
	PerceptionComponent->ConfigureSense(*AIItemSightConfig);
	PerceptionComponent->SetDominantSense(*AIItemSightConfig->GetSenseImplementation());

	PawnInventory = nullptr;

	bIsFiring = false;

	MovementBehavior = EAIPassiveState::PS_None;

	Target = nullptr;
	MovementTarget = nullptr;
	TargetControlRotation = FRotator::ZeroRotator;

	ReactionTime = 1.0f;


	MaxAimAttemps = 5;
	AimAttemptsLeft = MaxAimAttemps;
	AimAttemptFrequency = 500.0f;
	LastAimAttemptTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	LastAimModifier = FVector::ZeroVector;
	AimAccuracyStartingPoint = 200.0f;
}

FVector ACombatAIController::CalcFocalPointAccuracyModifier()
{
	const float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	if (CurrentTime - LastAimAttemptTime >= AimAttemptFrequency && IsAimedAtTarget())
	{
		AimAttemptsLeft = FMath::Clamp(AimAttemptsLeft, 0, MaxAimAttemps);

		LastAimAttemptTime = CurrentTime;

		LastAimModifier = FMath::VRand() * ((AimAccuracyStartingPoint / MaxAimAttemps) * AimAttemptsLeft);

		if (!LastAimModifier.Size())
		{
			LastAimModifier = FVector::ZeroVector;
		}

		FString LastAimModifierStr = FString::Printf(TEXT("%i Modifier is now %f %f %f %i"), AimAttemptsLeft, LastAimModifier.X, LastAimModifier.Y, LastAimModifier.Z, LastAimModifier.Size());
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple, *LastAimModifierStr);

		AimAttemptsLeft--;
	}

	if (!IsAimedAtTarget())
	{
		AimAttemptsLeft = MaxAimAttemps;
	}

	return LastAimModifier;
}

bool ACombatAIController::IsAimedAtTarget() const
{
	if (!Target)
	{
		return false;
	}

	if (APawn* CurrentPawn = GetPawn())
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		CurrentPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector DirectionToTarget = Target->GetActorLocation() - EyeLocation;
		FVector EyeForward = UKismetMathLibrary::GetForwardVector(EyeRotation);
		EyeForward.Normalize();
		DirectionToTarget.Normalize();

		FVector Difference = EyeForward - DirectionToTarget;

		// The angular distance between the look direction and the direction to a target at which the target considered to be aimed on
		static const float AimAngularDistance = 0.5f;

		return Difference.Size() <= AimAngularDistance && LineOfSightTo(GetTarget());
	}

	return false;
}

void ACombatAIController::InitDifficulty(EBotDifficulty InDifficulty)
{
	switch (InDifficulty)
	{
	case EBotDifficulty::Difficulty_Easy:
		Skill = FMath::RandRange(0, 3);
		Aggression = FMath::RandRange(0, 2);
		break;
	case EBotDifficulty::Difficulty_Medium:
		Skill = FMath::RandRange(3, 6);
		Aggression = FMath::RandRange(2, 4);
		break;
	case EBotDifficulty::Difficulty_Hard:
		Skill = FMath::RandRange(6, 10);
		Aggression = FMath::RandRange(4, 10);
		break;
	}

	Skill = 10;

	ReactionTime = 1.0f - Skill / 10.0f;
}

bool ACombatAIController::IsArmed() const
{
	if (PawnInventory)
	{
		return PawnInventory->GetEquippedFirearm() != nullptr;
	}

	return false;
}

void ACombatAIController::SetFiringEnabled(bool bEnabled)
{
	bIsFiring = bEnabled;
}

AActor* ACombatAIController::GetTarget() const
{
	return Target;
}

AActor* ACombatAIController::GetMovementTarget() const
{
	return MovementTarget;
}

void ACombatAIController::BeginPlay()
{
	Super::BeginPlay();

	// Probably best to prioritize based on the skill, but so far no need
	Tactics.Add(NewObject<UOffensiveAITactics>(this));
	Tactics.Add(NewObject<UDefensiveAITactics>(this));
	Tactics.Add(NewObject<UObjectiveAITactics>(this));

	for (UAITacticsBase* Tactic : Tactics)
	{
		Tactic->SetAIController(this);
	}
}

void ACombatAIController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	check(PerceptionComponent);
}

void ACombatAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACombatCharacter* CombatPawn = GetPawn<ACombatCharacter>();
	if (!CombatPawn || CombatPawn->IsDead())
	{
		return;
	}

	if (!GetWorldTimerManager().IsTimerActive(ReactionTimer))
	{
		SetTarget(FindClosestEnemy());

		for (UAITacticsBase* Tactic : Tactics)
		{
			if (Tactic->CanExecute())
			{
				Tactic->Execute();
			}
		}

		if (!IsAimedAtTarget())
		{
			SetFiringEnabled(false);
		}

		// Submit to blackboard
		if (Blackboard)
		{
			uint8 CurrentState = static_cast<uint8>(MovementBehavior);
			if (Blackboard->GetValueAsEnum(FName(TEXT("PassiveState"))) != CurrentState)
			{
				Blackboard->SetValueAsEnum(FName(TEXT("PassiveState")), CurrentState);
			}
		}

		GetWorldTimerManager().SetTimer(ReactionTimer, ReactionTime, false);
	}

	TickShootingTechnique();
}

void ACombatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ACombatCharacter* PossessedPawn = GetPawn<ACombatCharacter>())
	{
		PawnInventory = PossessedPawn->FindComponentByClass<UInventoryComponent>();

		if (BehaviorTree)
		{
			check(Blackboard);
			Blackboard->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

			check(BrainComponent);
			if (UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent))
			{
				BTComponent->StartTree(*BehaviorTree);
			}
		}

		EBotDifficulty Difficulty = static_cast<EBotDifficulty>(FMath::RandRange(0, 3));
		InitDifficulty(Difficulty);

		FString DifficultyStr = FString::Printf(TEXT("Applied a difficulty. Skill: %i, Aggression: %i"), Skill, Aggression);
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *DifficultyStr);
	}
}

void ACombatAIController::OnUnPossess()
{
	PawnInventory = nullptr;

	check(BrainComponent);
	if (UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent))
	{
		BTComponent->StopTree();
	}

	if (PerceptionComponent)
	{
		PerceptionComponent->ForgetAll();
	}

	Super::OnUnPossess();
}

void ACombatAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	if (APawn* const MyPawn = GetPawn())
	{
		TargetControlRotation = GetControlRotation();

		// Look toward focus
		const FVector FocalPoint = GetFocalPoint() + CalcFocalPointAccuracyModifier();
		DrawDebugPoint(GetWorld(), FocalPoint, 20.0f, FColor::Blue, true);

		if (FAISystem::IsValidLocation(FocalPoint))
		{
			TargetControlRotation = (FocalPoint - MyPawn->GetPawnViewLocation()).Rotation();
		}
		else if (bSetControlRotationFromPawnOrientation)
		{
			TargetControlRotation = MyPawn->GetActorRotation();
		}

		static const float BotRotationSpeed = 5.0f;

		FRotator RotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(GetControlRotation(), TargetControlRotation);
		FRotator LerpedRotation = FMath::RInterpTo(GetControlRotation(), TargetControlRotation, RotationDelta.Vector().Size() * BotRotationSpeed * DeltaTime, 1.0f);

		SetControlRotation(LerpedRotation);

		if (bUpdatePawn)
		{
			const FRotator CurrentPawnRotation = MyPawn->GetActorRotation();

			if (CurrentPawnRotation.Equals(LerpedRotation, 1e-3f) == false)
			{
				MyPawn->FaceRotation(LerpedRotation, DeltaTime);
			}
		}
	}
}

static float LastFireTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());

void ACombatAIController::TickShootingTechnique()
{
	// Set parameters
	MaxAimAttemps = 10 - Skill;
	AimAttemptFrequency = 1100.0f - Skill * 100.0f;
	AimAccuracyStartingPoint = 200 - 20.0f * Skill;

	int32 Burst = 6;
	int32 CurrentBurst = 0;

	float FireFrequency = 600.0f;

	ACombatCharacter* CombatPawn = GetPawn<ACombatCharacter>();
	AFirearm* PrimaryFirearm = PawnInventory->GetEquippedFirearm();

	if (!CombatPawn || !PrimaryFirearm)
	{
		return;
	}

	if (PrimaryFirearm->IsReloading())
	{
		BurstShotsMade = 0;
	}

	// Tick firing
	if (bIsFiring)
	{
		switch (PrimaryFirearm->CurrentFireMode)
		{
		case EFirearmFireMode::Auto:
			{
				float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
				if (CurrentTime - LastFireTime >= FireFrequency)
				{
					if (!PrimaryFirearm->IsReloading() && !PrimaryFirearm->IsFiring())
					{
						BurstStartAmmo = PrimaryFirearm->CurrentMagAmmo;
						CombatPawn->StartPrimaryFire();
					}
				}

				BurstShotsMade = BurstStartAmmo - PrimaryFirearm->CurrentMagAmmo;
				/*FString BurstShotsMadeStr = FString::Printf(TEXT("%i"), BurstShotsMade);
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple, *BurstShotsMadeStr);*/

				if (BurstShotsMade >= Burst)
				{
					CombatPawn->StopPrimaryFire();
					LastFireTime = CurrentTime;
					BurstShotsMade = 0;
					BurstStartAmmo = PrimaryFirearm->CurrentMagAmmo;
				}

				break;
			}	
		case EFirearmFireMode::SemiAuto:
			if (!PrimaryFirearm->IsReloading())
			{
				float CurrentTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
				if (CurrentTime - LastFireTime >= FireFrequency)
				{
					CombatPawn->StopPrimaryFire();
					CombatPawn->StartPrimaryFire();
					LastFireTime = CurrentTime;
				}
			}
			break;
		}

		if (PrimaryFirearm->CurrentMagAmmo <= 0)
		{
			CombatPawn->StopPrimaryFire();
			CombatPawn->Reload();
		}
	}
	else
	{
		CombatPawn->StopPrimaryFire();
	}

	// Reload when no targets
	if (!GetTarget())
	{
		if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
		{
			if (CombatPawn)
			{
				CombatPawn->Reload();
			}
		}
	}
}

bool ACombatAIController::IsAmmoLow() const
{
	if (!PawnInventory)
	{
		return true;
	}

	TArray<const AFirearm*> Firearms;
	Firearms.Add(PawnInventory->GetFirearm(EInventorySlot::Primary));
	Firearms.Add(PawnInventory->GetFirearm(EInventorySlot::Sidearm));

	int32 TotalAmmo = 0;
	int32 PrimaryAmmo = 0;

	for (const AFirearm* Firearm : Firearms)
	{
		if (!Firearm)
		{
			continue;
		}

		TotalAmmo += Firearm->CurrentMagAmmo + Firearm->CurrentReserveAmmo;
		if (Firearm->Slot == EInventorySlot::Primary)
		{
			PrimaryAmmo += Firearm->CurrentMagAmmo + Firearm->CurrentReserveAmmo;
		}
	}

	int32 MinimumAmmo = 30;
	if (AFirearm* EquippedFirearm = PawnInventory->GetEquippedFirearm())
	{
		MinimumAmmo = EquippedFirearm->MagAmmoCapacity;
	}

	if (TotalAmmo < MinimumAmmo)
	{
		return true;
	}

	return false;
}

UInventoryComponent* ACombatAIController::GetPawnInventory() const
{
	return PawnInventory;
}

void ACombatAIController::EquipBestWeapon()
{
	if (!PawnInventory)
	{
		return;
	}

	EInventorySlot ChosenSlot = EInventorySlot::Melee;

	for (int8 i = static_cast<int8>(EInventorySlot::Sidearm); i >= 0; --i)
	{
		if (AFirearm* Weapon = PawnInventory->GetFirearm(static_cast<EInventorySlot>(i)))
		{
			if (Weapon->CurrentMagAmmo + Weapon->CurrentReserveAmmo >= Weapon->MagAmmoCapacity)
			{
				ChosenSlot = Weapon->Slot;
			}
		}
	}

	PawnInventory->EquipFirearm(ChosenSlot);
}

bool ACombatAIController::IsReloading() const
{
	if (PawnInventory)
	{
		if (AFirearm* EquippedWeapon = PawnInventory->GetEquippedFirearm())
		{
			if (EquippedWeapon->IsReloading())
			{
				return true;
			}
		}
	}

	return false;
}

void ACombatAIController::React(EReaction InReaction)
{
	if (FReactionInfo* ReactionInfo = Reactions.FindByPredicate([InReaction](const FReactionInfo& ReactionInfo)
	{
		return ReactionInfo.Reaction == InReaction;
	}))
	{
		const int32 CurrentSkill = Skill;
		const int32 CurrentAggression = Aggression;
		if (FReactionResponse* ReactionResponse = ReactionInfo->Responses.FindByPredicate([CurrentSkill, CurrentAggression](const FReactionResponse& ReactionResponse)
		{
			return ReactionResponse.SkillRequired <= CurrentSkill && ReactionResponse.AggressionRequired <= CurrentAggression;
		}))
		{
			SetMovementBehavior(ReactionResponse->MovementBehavior);
			return;
		}
	}

	SetMovementBehavior(EAIPassiveState::PS_None);
}

AFirearm* ACombatAIController::GetPawnWeapon() const
{
	if (PawnInventory)
	{
		return PawnInventory->GetEquippedFirearm();
	}

	return nullptr;
}

void ACombatAIController::SetTarget(AActor* NewTarget)
{
	Target = NewTarget;

	if (Target)
	{
		SetFocus(Target);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	Blackboard->SetValueAsObject(FName(TEXT("Target")), Target);
}

void ACombatAIController::SetMovementTarget(AActor* NewTarget)
{
	if (MovementTarget == NewTarget)
	{
		return;
	}

	MovementTarget = NewTarget;
	Blackboard->SetValueAsObject(FName(TEXT("MovementTarget")), MovementTarget);
}

void ACombatAIController::SetMovementBehavior(EAIPassiveState NewState)
{
	MovementBehavior = NewState;
}

AActor* ACombatAIController::FindClosestEnemy() const
{
	check(PerceptionComponent);

	TArray<AActor*> EnemiesInSight;

	// Iterate over all VISIBLE percepted actors
	for (auto Iter = PerceptionComponent->GetPerceptualDataConstIterator(); Iter; ++Iter)
	{
		AActor* PerceptedActor = Iter.Key().ResolveObjectPtr();
		const FActorPerceptionInfo& PerceptionInfo = Iter.Value();

		if (PerceptedActor && PerceptionInfo.HasAnyKnownStimulus())
		{
			// Check if the actor has any affiliation info
			if (const IGenericTeamAgentInterface* TeamInterface = Cast<const IGenericTeamAgentInterface>(PerceptedActor))
			{
				if (TeamInterface->GetTeamAttitudeTowards(*GetPawn()) == ETeamAttitude::Hostile)
				{
					EnemiesInSight.Add(PerceptedActor);
				}
			}
		}
	}

	FVector PawnLocation;

	if (APawn* AIPawn = GetPawn())
	{
		PawnLocation = AIPawn->GetActorLocation();
	}

	// Sort over distance
	EnemiesInSight.Sort([PawnLocation](AActor& ActorA, AActor& ActorB) {
		return FVector::Distance(ActorA.GetActorLocation(), PawnLocation) < FVector::Distance(ActorB.GetActorLocation(), PawnLocation);
	});

	if (EnemiesInSight.Num() > 0)
	{
		return EnemiesInSight[0];
	}

	return nullptr;
}
