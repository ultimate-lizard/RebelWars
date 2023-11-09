#include "AI/Tactics/DefensiveAITactics.h"

#include "Controllers/CombatAIController.h"
#include "Components/InventoryComponent.h"
#include "Components/InteractableComponent.h"
#include "Items/Firearm.h"
#include "Characters/CombatCharacter.h"

bool UDefensiveAITactics::CanExecute() const
{
    if (!AIController)
    {
        return false;
    }

    bool bCanExecute = AIController->IsAmmoLow();
	bCanExecute |= AIController->IsReloading();

	if (ACombatCharacter* CombatPawn = AIController->GetPawn<ACombatCharacter>())
	{
		bCanExecute |= CombatPawn->GetCurrentHealth() < 20.0f;
	}

    return bCanExecute;
}

void UDefensiveAITactics::Execute()
{
    if (!AIController)
    {
        return;
    }

	// TODO: Find better solution
	// Prevent AI to pursue already occupied weapon
	if (AFirearm* TargetWeapon = Cast<AFirearm>(AIController->GetMovementTarget()))
	{
		if (TargetWeapon->GetOwner())
		{
			if (UAIPerceptionComponent* PerceptionComponent = AIController->GetPerceptionComponent())
			{
				PerceptionComponent->ForgetActor(TargetWeapon);
			}

			AIController->SetMovementTarget(nullptr);
			AIController->SetMovementBehavior(EAIPassiveState::PS_None);
		}
	}

	if (AIController->IsReloading())
	{
		AIController->React(EReaction::Reaction_Reload);
	}

	if (ACombatCharacter* CombatPawn = AIController->GetPawn<ACombatCharacter>())
	{
		if (CombatPawn->GetCurrentHealth() < 20.0f)
		{
			AIController->React(EReaction::Reaction_LowHealth);
		}
	}

	if (APawn* PossessedPawn = AIController->GetPawn())
	{
		if (AIController->IsAmmoLow())
		{
			if (AFirearm* WeaponInSight = AIController->FindClosestSensedActor<AFirearm>())
			{
				if (AIController->GetTarget() != WeaponInSight)
				{
					AIController->SetTarget(WeaponInSight);
				}

				bool bCloseEnoughToPickup = FVector::Distance(PossessedPawn->GetActorLocation(), WeaponInSight->GetActorLocation()) <= 200.0f;
				if (bCloseEnoughToPickup)
				{
					// Hardcoded interaction logic. No need to be a separate logic for now, for it's only used here
					if (UInteractableComponent* Interactable = WeaponInSight->FindComponentByClass<UInteractableComponent>())
					{
						Interactable->Interact(PossessedPawn);
						AIController->SetMovementTarget(nullptr);
						AIController->SetTarget(nullptr);
					}
				}
				else
				{
					// Reaction See Loot
					AIController->SetMovementTarget(WeaponInSight);
					AIController->SetMovementBehavior(EAIPassiveState::PS_MoveToTarget);
				}
			}
			else
			{
				AIController->React(EReaction::Reaction_NoAmmoInSight);
			}
		}
	}
}
