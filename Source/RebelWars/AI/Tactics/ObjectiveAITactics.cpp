#include "AI/Tactics/ObjectiveAITactics.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "GameModes/RWGameModeBase.h"
#include "Controllers/CombatAIController.h"

bool UObjectiveAITactics::CanExecute() const
{
    return true;
}

void UObjectiveAITactics::Execute()
{
    if (!AIController)
    {
        return;
    }

    if (AGameModeBase* CurrentGameMode = UGameplayStatics::GetGameMode(GetWorld()))
    {
        if (CurrentGameMode->IsA<ARWGameModeBase>())
        {
            if (!AIController->GetTarget() && !AIController->GetMovementTarget())
            {
                AIController->SetMovementBehavior(EAIPassiveState::PS_Patrol);
            }
        }
    }
}
