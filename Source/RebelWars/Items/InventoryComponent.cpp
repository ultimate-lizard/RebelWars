#include "Items/InventoryComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Firearm.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, PrimaryFirearm);
}

void UInventoryComponent::OnRep_PrimaryFirearm()
{
	if (PrimaryFirearm)
	{
		SimulatedPrimaryFirearm = PrimaryFirearm;
		PickupFirearm(PrimaryFirearm, true);
	}
	else
	{
		DropFirearm(SimulatedPrimaryFirearm, true);
		SimulatedPrimaryFirearm = nullptr;
	}
}

AFirearm* UInventoryComponent::GetPrimaryFirearm()
{
	return PrimaryFirearm;
}

void UInventoryComponent::PickupFirearm(AFirearm* InFirearm, bool bFromReplication)
{
	if (!bFromReplication)
	{
		if (GetOwnerRole() < ENetRole::ROLE_Authority)
		{
			ServerPickupFirearm(InFirearm);
			return;
		}

		if (GetOwnerRole() == ENetRole::ROLE_Authority)
		{
			if (!InFirearm)
			{
				return;
			}

			PrimaryFirearm = InFirearm;
		}
	}

	InFirearm->Pickup(this);

	OnFirearmPickupDelegate.Broadcast(InFirearm);
}

void UInventoryComponent::DropFirearm(AFirearm* InFirearm, bool bFromReplication)
{
	if (!bFromReplication)
	{
		if (GetOwnerRole() < ENetRole::ROLE_Authority)
		{
			ServerDropFirearm(InFirearm);
			return;
		}

		if (GetOwnerRole() == ENetRole::ROLE_Authority)
		{
			if (!InFirearm || InFirearm->IsFiring() || InFirearm->IsReloading())
			{
				return;
			}

			PrimaryFirearm = nullptr;
		}
	}
	
	InFirearm->Drop();

	OnFirearmDropDelegate.Broadcast(InFirearm);
}

void UInventoryComponent::ServerPickupFirearm_Implementation(AFirearm* InFirearm)
{
	PickupFirearm(InFirearm);
}

void UInventoryComponent::ServerDropFirearm_Implementation(AFirearm* InFirearm)
{
	DropFirearm(InFirearm);
}
