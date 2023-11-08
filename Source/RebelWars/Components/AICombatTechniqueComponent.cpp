#include "Components/AICombatTechniqueComponent.h"

UAICombatTechniqueComponent::UAICombatTechniqueComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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
}

void UAICombatTechniqueComponent::UpdateAIControlRotation()
{
}
