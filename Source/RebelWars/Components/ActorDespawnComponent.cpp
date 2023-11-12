#include "Components/ActorDespawnComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Items/Firearm.h"

TArray<AActor*> UActorDespawnComponent::DespawnQueue;

UActorDespawnComponent::UActorDespawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	DespawnLimit = 5;
}

void UActorDespawnComponent::AddToDespawnQueue(AActor* ActorToAdd)
{
	if (GetOwnerRole() < ENetRole::ROLE_Authority)
	{
		return;
	}

	if (!GetOwner())
	{
		return;
	}

	DespawnQueue.Add(ActorToAdd);

	if (DespawnQueue.Num() > DespawnLimit)
	{
		if (AActor* FirstActor = DespawnQueue[0])
		{
			if (!FirstActor->IsPendingKill())
			{
				FirstActor->Destroy();
			}
		}

		DespawnQueue.RemoveAt(0);
	}
}

void UActorDespawnComponent::SetQueueLimit(uint32 NewLimit)
{
	DespawnLimit = NewLimit;
}

void UActorDespawnComponent::FreeDespawnSlot(AActor* ActorToFree)
{
	if (GetOwnerRole() < ENetRole::ROLE_Authority)
	{
		return;
	}

	DespawnQueue.Remove(ActorToFree);
}
