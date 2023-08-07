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
#include "Items/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/CombatCharacter.h"

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
}

void ACombatAIController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	check(PerceptionComponent);
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
}

void ACombatAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFiring)
	{
		if (ACombatCharacter* CombatPawn = Cast<ACombatCharacter>(GetPawn()))
		{
			if (UInventoryComponent* PawnInventory = CombatPawn->FindComponentByClass<UInventoryComponent>())
			{
				if (AFirearm* PrimaryFirearm = PawnInventory->GetPrimaryFirearm())
				{
					if (PrimaryFirearm->CurrentMagAmmo <= 0)
					{
						// TODO: Implement reaction time
						CombatPawn->Reload();
					}
				}
			}
		}
	}
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

void ACombatAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	if (Actor->GetClass()->IsChildOf(AFirearm::StaticClass()))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			FirearmsInSight.Add(Actor);
		}
		else
		{
			FirearmsInSight.Remove(Actor);
		}
	}
	else if (Actor->GetClass()->IsChildOf(APawn::StaticClass()))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			PawnsInSight.Add(Actor);
		}
		else
		{
			PawnsInSight.Remove(Actor);
		}

		SetNewTargetEnemy(EvaluateTargetEnemy());
	}
}

bool ACombatAIController::IsCharacterArmed() const
{
	if (ACombatCharacter* CombatCharacter = Cast<ACombatCharacter>(GetCharacter()))
	{
		return CombatCharacter->IsArmed();
	}

	return false;
}

void ACombatAIController::SetNewTargetEnemy(APawn* InPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TargetEnemy = InPawn;

	if (InPawn)
	{
		RememberedTargetEnemy = InPawn;
		World->GetTimerManager().ClearTimer(MemoryTimer);
		MemoryTimer.Invalidate();
	}
	else
	{
		// AI still remembers the last enemy for some time
		if (RememberedTargetEnemy)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Lost target. Chasing by memory"));

			World->GetTimerManager().SetTimer(MemoryTimer, this, &ACombatAIController::ForgetTargetEnemy, TargetMemoryLength, false);
		}
	}
}

APawn* ACombatAIController::GetTargetEnemy()
{
	return TargetEnemy;
}

APawn* ACombatAIController::GetRememberedTargetEnemy()
{
	return RememberedTargetEnemy;
}

APawn* ACombatAIController::EvaluateTargetEnemy() const
{
	TArray<TSoftObjectPtr<APawn>> EnemiesInSight;
	for (const TSoftObjectPtr<APawn>& PawnInSight : PawnsInSight)
	{
		if (const IGenericTeamAgentInterface* TeamInterface = Cast<const IGenericTeamAgentInterface>(PawnInSight.Get()))
		{
			if (APawn* CurrentPawn = GetPawn())
			{
				if (TeamInterface->GetTeamAttitudeTowards(*CurrentPawn) == ETeamAttitude::Hostile)
				{
					EnemiesInSight.Add(PawnInSight.Get());
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("Adding: %s"), *PawnInSight.Get()->GetName()));
					// TODO: Implement evaluation from the array of enemies
				}
			}
		}
	}

	if (EnemiesInSight.Num() > 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Evaluated a target"));
		return EnemiesInSight[0].Get();
	}

	return nullptr;
}

void ACombatAIController::ForgetTargetEnemy()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Forgot the enemy"));

	RememberedTargetEnemy = nullptr;
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MemoryTimer);
		MemoryTimer.Invalidate();
	}
}

const TArray<TSoftObjectPtr<AFirearm>>& ACombatAIController::GetFirearmsInSight() const
{
	return FirearmsInSight;
}

const TArray<TSoftObjectPtr<APawn>>& ACombatAIController::GetPawnsInSight() const
{
	return PawnsInSight;
}

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

bool ACombatAIController::CanSeeTarget(AActor* InActor) const
{
	return PawnsInSight.Find(InActor) != INDEX_NONE;
}
