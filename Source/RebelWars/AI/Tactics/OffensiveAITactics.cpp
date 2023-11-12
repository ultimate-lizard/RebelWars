#include "AI/Tactics/OffensiveAITactics.h"

#include "Controllers/CombatAIController.h"
#include "Components/InventoryComponent.h"
#include "Items/Firearm.h"

bool UOffensiveAITactics::CanExecute() const
{
    if (!AIController)
    {
        return false;
    }

    bool bCanExecute = true;

    bCanExecute &= !AIController->IsReloading();
    bCanExecute &= AIController->GetTarget() != nullptr;
    bCanExecute &= !AIController->IsAmmoLow();

    return bCanExecute;
}

void UOffensiveAITactics::Execute()
{
    if (!AIController)
    {
        return;
    }

    AIController->EquipBestWeapon();
    
    if (AIController->GetTarget())
    {
        AIController->React(EReaction::Reaction_HasTarget);
    }
}
