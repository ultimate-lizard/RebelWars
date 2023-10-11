#include "Components/InteractableComponent.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Prompt = FString(TEXT("Use"));
}

void UInteractableComponent::Interact(AActor* Initiator)
{
	OnInteract.Broadcast(Initiator);
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
}
