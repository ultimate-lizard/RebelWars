#include "Controllers/CombatAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Items/Firearm.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/CombatCharacter.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "Perception/AIPerceptionComponent.h"
#include "AI/Tactics/OffensiveAITactics.h"
#include "AI/Tactics/DefensiveAITactics.h"
#include "AI/Tactics/ObjectiveAITactics.h"

ACombatAIController::ACombatAIController()
{
	Blackboard = CreateDefaultSubobject<UBlackboardComponent>(FName(TEXT("Blackboard")));
	BrainComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(FName(TEXT("Behavior Tree")));
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(FName(TEXT("AI Perception")));
	check(PerceptionComponent);

	AIItemSightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(FName(TEXT("AI Sight Config")));
	check(AIItemSightConfig);
	PerceptionComponent->ConfigureSense(*AIItemSightConfig);
	PerceptionComponent->SetDominantSense(*AIItemSightConfig->GetSenseImplementation());

	bWantsPlayerState = true;

	bIsFiring = false;

	Target = nullptr;
	MovementTarget = nullptr;

	ReactionTime = 1.0f;
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

		if (!LineOfSightTo(GetTarget()))
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
		FRotator NewControlRotation = GetControlRotation();

		// Look toward focus
		const FVector FocalPoint = GetFocalPoint();
		if (FAISystem::IsValidLocation(FocalPoint))
		{
			NewControlRotation = (FocalPoint - MyPawn->GetPawnViewLocation()).Rotation();
		}
		else if (bSetControlRotationFromPawnOrientation)
		{
			NewControlRotation = MyPawn->GetActorRotation();
		}

		SetControlRotation(NewControlRotation);

		if (bUpdatePawn)
		{
			const FRotator CurrentPawnRotation = MyPawn->GetActorRotation();

			if (CurrentPawnRotation.Equals(NewControlRotation, 1e-3f) == false)
			{
				MyPawn->FaceRotation(NewControlRotation, DeltaTime);
			}
		}
	}
}

void ACombatAIController::TickShootingTechnique()
{
	bool bUseBurst = Skill >= 5;
	int32 ShotsPerBurst = 3;
	float Accuracy = 10.0f; // Worst accuracy

	ACombatCharacter* CombatPawn = GetPawn<ACombatCharacter>();
	AFirearm* PrimaryFirearm = PawnInventory->GetEquippedFirearm();

	if (!CombatPawn || !PrimaryFirearm)
	{
		return;
	}

	if (bIsFiring)
	{
		switch (PrimaryFirearm->CurrentFireMode)
		{
		case EFirearmFireMode::Auto:
			if (!PrimaryFirearm->IsReloading() && !PrimaryFirearm->IsFiring())
			{
				CombatPawn->StartPrimaryFire();
			}
			break;
		case EFirearmFireMode::SemiAuto:
			if (!PrimaryFirearm->IsReloading())
			{
				CombatPawn->StopPrimaryFire();
				CombatPawn->StartPrimaryFire();
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
