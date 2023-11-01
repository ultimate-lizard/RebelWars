#include "Controllers/CombatAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Items/Firearm.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/CombatCharacter.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "Perception/AIPerceptionComponent.h"

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

	TargetMemoryLength = 5.0f;

	bWantsPlayerState = true;

	//bCanSeeAWeapon = false;
	//TargetEnemy = nullptr;
	//RememberedTargetEnemy = nullptr;

	bIsFiring = false;
}

void ACombatAIController::BeginPlay()
{
	Super::BeginPlay();

	// IsArmedKey = Blackboard->GetKeyID(FName(TEXT("IsArmed")));
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
	if (!CombatPawn)
	{
		return;
	}

	// Think
	if (!CombatPawn->IsDead())
	{
		Think();
	}

	// Update weapon status for blackboard
	//if (APawn* CurrentPawn = GetPawn())
	//{
	//	if (UInventoryComponent* PawnInventory = CurrentPawn->FindComponentByClass<UInventoryComponent>())
	//	{
	//		Blackboard->SetValue<UBlackboardKeyType_Bool>(IsArmedKey, PawnInventory->GetEquippedFirearm() != nullptr);
	//	}
	//}
}

void ACombatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ACombatCharacter* PossessedPawn = GetPawn<ACombatCharacter>())
	{
		PawnInventory = PossessedPawn->FindComponentByClass<UInventoryComponent>();

		if (UBehaviorTree* BehaviorTree = PossessedPawn->BehaviorTree)
		{
			check(Blackboard);
			Blackboard->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

			check(BrainComponent);
			if (UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent))
			{
				BTComponent->StartTree(*BehaviorTree);
			}
		}
	}

	// CachedInventory
}

void ACombatAIController::OnUnPossess()
{
	PawnInventory = nullptr;

	check(BrainComponent);
	if (UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent))
	{
		BTComponent->StopTree();
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

void ACombatAIController::Think()
{
	UpdateAttacking();
	UpdateInventoryManaging();
}

void ACombatAIController::UpdateInventoryManaging()
{
	if (!PawnInventory)
	{
		return;
	}

	APawn* PossessedPawn = GetPawn();
	if (!PossessedPawn)
	{
		return;
	}

	EquipBestWeapon(*PawnInventory);

	if (IsAmmoLow(*PawnInventory))
	{
		if (AFirearm* WeaponInSight = GetClosestSensedActor<AFirearm>())
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, *WeaponInSight->GetName());
			bool bIsCloseEnoughToWeapon = FVector::Distance(PossessedPawn->GetActorLocation(), WeaponInSight->GetActorLocation()) <= 200.0f;
			if (bIsCloseEnoughToWeapon)
			{
				//SetTarget(WeaponInSight);
				//UpdateActiveState(EAIActiveState::AS_PickupItem);
				if (UInteractableComponent* Interactable = WeaponInSight->FindComponentByClass<UInteractableComponent>())
				{
					Interactable->Interact(PossessedPawn);
				}
			}
			else
			{
				SetMovementTarget(WeaponInSight);
				UpdatePassiveState(EAIPassiveState::PS_MoveToTarget);
			}
		}
		else
		{
			UpdatePassiveState(EAIPassiveState::PS_Patrol);
		}
	}
}

void ACombatAIController::UpdateAttacking()
{
	if (!PawnInventory)
	{
		return;
	}

	if (PawnInventory->GetEquippedFirearm())
	{
		// Update reloading state
		if (AFirearm* EquippedFirearm = PawnInventory->GetEquippedFirearm())
		{
			if (EquippedFirearm->IsReloading())
			{
				// TODO: EVALUATE SKILL
				UpdatePassiveState(EAIPassiveState::PS_TakeCover);
				return;
			}
		}

		// Find target
		if (AActor* Enemy = FindClosestEnemy())
		{
			// TODO: Evaluate items and aggression
			SetTarget(Enemy);
			// UpdateActiveState(EAIActiveState::AS_Shoot);

			// TODO: Evaluate skill, agression
			UpdatePassiveState(EAIPassiveState::PS_Push);
		}
		else
		{
			SetTarget(nullptr);
			// UpdateActiveState(EAIActiveState::AS_Idle);
			UpdatePassiveState(EAIPassiveState::PS_Patrol);
			if (PawnInventory)
			{
				if (AFirearm* EquippedWeapon = PawnInventory->GetEquippedFirearm())
				{
					EquippedWeapon->Reload();
				}
			}
		}

		// Toggle state
		if (Target && LineOfSightTo(Target) && !bIsFiring)
		{
			StartFireAt(Target);
		}
		else
		{
			StopFire();
		}

		// Update state
		if (ACombatCharacter* CombatPawn = GetPawn<ACombatCharacter>())
		{
			// Update attack state
			if (bIsFiring)
			{
				if (AFirearm* PrimaryFirearm = PawnInventory->GetEquippedFirearm())
				{
					if (!PrimaryFirearm->IsReloading() && !PrimaryFirearm->IsFiring())
					{
						CombatPawn->StartPrimaryFire();
					}

					if (PrimaryFirearm->CurrentMagAmmo <= 0)
					{
						CombatPawn->StopPrimaryFire();
						CombatPawn->Reload();
					}
				}
			}
		}
	}
}

bool ACombatAIController::IsAmmoLow(const UInventoryComponent& Inventory)
{
	TArray<const AFirearm*> Firearms;
	Firearms.Add(Inventory.GetFirearm(EInventorySlot::Primary));
	Firearms.Add(Inventory.GetFirearm(EInventorySlot::Sidearm));

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

	if (TotalAmmo < 30)
	{
		return true;
	}
	// TODO: CHECK TOTAL AMMO FOR LOW SKILLS

	// TODO: CHECK PRIMARY FOR HIGH SKILLS

	return false;
}

void ACombatAIController::EquipBestWeapon(UInventoryComponent& Inventory)
{
	EInventorySlot ChosenSlot = EInventorySlot::Melee;

	for (int8 i = static_cast<int8>(EInventorySlot::Sidearm); i >= 0; --i)
	{
		if (AFirearm* Weapon = Inventory.GetFirearm(static_cast<EInventorySlot>(i)))
		{
			if (Weapon->CurrentMagAmmo + Weapon->CurrentReserveAmmo >= Weapon->MagAmmoCapacity)
			{
				ChosenSlot = Weapon->Slot;
			}
		}
	}

	Inventory.EquipFirearm(ChosenSlot);
}

void ACombatAIController::SetTarget(AActor* NewTarget)
{
	if (Target == NewTarget)
	{
		return;
	}

	Target = NewTarget;
	SetFocus(Target);
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

//void ACombatAIController::UpdateActiveState(EAIActiveState NewState)
//{
//	if (CurrentActiveState == NewState)
//	{
//		return;
//	}
//
//	CurrentActiveState = NewState;
//	Blackboard->SetValueAsEnum(FName(TEXT("ActiveState")), static_cast<uint8>(CurrentActiveState));
//}

void ACombatAIController::UpdatePassiveState(EAIPassiveState NewState)
{
	if (CurrentPassiveState == NewState)
	{
		return;
	}

	// TODO: Find place for names
	CurrentPassiveState = NewState;
	Blackboard->SetValueAsEnum(FName(TEXT("PassiveState")), static_cast<uint8>(CurrentPassiveState));
}

//bool ACombatAIController::IsCharacterArmed() const
//{
//	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(GetCharacter()))
//	{
//		return CombatCharacter->IsArmed();
//	}
//
//	return false;
//}

//void ACombatAIController::SetNewTargetEnemy(APawn* InPawn)
//{
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return;
//	}
//
//	TargetEnemy = InPawn;
//
//	if (InPawn)
//	{
//		RememberedTargetEnemy = InPawn;
//		World->GetTimerManager().ClearTimer(MemoryTimer);
//		MemoryTimer.Invalidate();
//	}
//	else
//	{
//		// AI still remembers the last enemy for some time
//		if (RememberedTargetEnemy)
//		{
//			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Lost target. Chasing by memory"));
//
//			World->GetTimerManager().SetTimer(MemoryTimer, this, &ACombatAIController::ForgetTargetEnemy, TargetMemoryLength, false);
//		}
//	}
//
//	// PerceptionComponent->ForgetActor
//}

//APawn* ACombatAIController::GetTargetEnemy()
//{
//	return TargetEnemy;
//}

//APawn* ACombatAIController::GetRememberedTargetEnemy()
//{
//	return RememberedTargetEnemy;
//}

//APawn* ACombatAIController::EvaluateTargetEnemy() const
//{
//	TArray<TSoftObjectPtr<APawn>> EnemiesInSight;
//	//for (const TSoftObjectPtr<APawn>& PawnInSight : PawnsInSight)
//	//{
//	//	if (const IGenericTeamAgentInterface* TeamInterface = Cast<const IGenericTeamAgentInterface>(PawnInSight.Get()))
//	//	{
//	//		if (APawn* CurrentPawn = GetPawn())
//	//		{
//	//			if (TeamInterface->GetTeamAttitudeTowards(*CurrentPawn) == ETeamAttitude::Hostile)
//	//			{
//	//				EnemiesInSight.Add(PawnInSight.Get());
//	//				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("Adding: %s"), *PawnInSight.Get()->GetName()));
//	//				// TODO: Implement evaluation from the array of enemies
//	//			}
//	//		}
//	//	}
//	//}
//
//	if (EnemiesInSight.Num() > 0)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Evaluated a target"));
//		return EnemiesInSight[0].Get();
//	}
//
//	return nullptr;
//}

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

//void ACombatAIController::ForgetTargetEnemy()
//{
//	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Forgot the enemy"));
//
//	RememberedTargetEnemy = nullptr;
//	
//	if (UWorld* World = GetWorld())
//	{
//		World->GetTimerManager().ClearTimer(MemoryTimer);
//		MemoryTimer.Invalidate();
//	}
//}

void ACombatAIController::StartFireAt(AActor* InActor)
{
	//SetFocus(InActor);

	bIsFiring = true;

	if (ACombatCharacter* CombatPawn = Cast<ACombatCharacter>(GetPawn()))
	{
		CombatPawn->StartPrimaryFire();
	}
}

void ACombatAIController::StopFire()
{
	//SetFocus(nullptr);

	bIsFiring = false;

	if (ACombatCharacter* CombatPawn = Cast<ACombatCharacter>(GetPawn()))
	{
		CombatPawn->StopPrimaryFire();
	}
}
