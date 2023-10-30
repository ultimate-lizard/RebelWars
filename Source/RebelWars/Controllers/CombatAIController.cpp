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

	bCanSeeAWeapon = false;
	TargetEnemy = nullptr;
	RememberedTargetEnemy = nullptr;

	FireTarget = nullptr;
	bIsFiring = false;
}

void ACombatAIController::BeginPlay()
{
	Super::BeginPlay();

	IsArmedKey = Blackboard->GetKeyID(FName(TEXT("IsArmed")));
}

void ACombatAIController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	check(PerceptionComponent);
}

void ACombatAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO: If alive
	Think();

	if (bIsFiring)
	{
		if (ACombatCharacter* CombatPawn = Cast<ACombatCharacter>(GetPawn()))
		{
			if (UInventoryComponent* PawnInventory = CombatPawn->FindComponentByClass<UInventoryComponent>())
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

	// Update weapon status for blackboard
	if (APawn* CurrentPawn = GetPawn())
	{
		if (UInventoryComponent* PawnInventory = CurrentPawn->FindComponentByClass<UInventoryComponent>())
		{
			Blackboard->SetValue<UBlackboardKeyType_Bool>(IsArmedKey, PawnInventory->GetEquippedFirearm() != nullptr);
		}
	}

	PerceptionComponent->GetPerceptualDataConstIterator();
}

void ACombatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTree)
	{
		return;
	}

	check(Blackboard);
	Blackboard->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

	check(BrainComponent);
	if (UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent))
	{
		BTComponent->StartTree(*BehaviorTree);
	}
}

void ACombatAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
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
	CheckInventory();
}

void ACombatAIController::CheckInventory()
{
	if (APawn* PossessedPawn = GetPawn())
	{
		if (UInventoryComponent* Inventory = PossessedPawn->FindComponentByClass<UInventoryComponent>())
		{
			if (IsAmmoLow(*Inventory))
			{
				// if (has weapons in range)
				// else

				if (CurrentPassiveState != EAIPassiveState::PS_MoveToTarget)
				{
					if (AFirearm* FoundWeapon = GetClosestSensedActor<AFirearm>())
					{
						SetMovementTarget(FoundWeapon);
						UpdatePassiveState(EAIPassiveState::PS_MoveToTarget);
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

		TotalAmmo += Firearm->CurrentMagAmmo + Firearm->ReserveAmmoCapacity;
		if (Firearm->Slot == EInventorySlot::Primary)
		{
			PrimaryAmmo += Firearm->CurrentMagAmmo + Firearm->ReserveAmmoCapacity;
		}
	}

	// TODO: CHECK TOTAL AMMO FOR LOW SKILLS

	// TODO: CHECK PRIMARY FOR HIGH SKILLS

	return true;
}

void ACombatAIController::SetMovementTarget(AActor* NewTarget)
{
	MovementTarget = NewTarget;
	Blackboard->SetValueAsObject(FName(TEXT("MovementTarget")), MovementTarget);
}

void ACombatAIController::UpdatePassiveState(EAIPassiveState NewState)
{
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

		if (PerceptedActor && PerceptionInfo.HasAnyCurrentStimulus())
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
	SetFocus(InActor);

	FireTarget = InActor;
	bIsFiring = true;

	if (ACombatCharacter* CombatPawn = Cast<ACombatCharacter>(GetPawn()))
	{
		CombatPawn->StartPrimaryFire();
	}
}

void ACombatAIController::StopFire()
{
	SetFocus(nullptr);

	FireTarget = nullptr;
	bIsFiring = false;

	if (ACombatCharacter* CombatPawn = Cast<ACombatCharacter>(GetPawn()))
	{
		CombatPawn->StopPrimaryFire();
	}
}
