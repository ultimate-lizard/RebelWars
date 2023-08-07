#include "Items/InventoryComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Firearm.h"

UInventoryComponent::UInventoryComponent()
{
	// PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, PrimaryFirearm);
}

void UInventoryComponent::PickupFirearm_Internal(AFirearm* InFirearm)
{
	if (!InFirearm)
	{
		return;
	}

	InFirearm->Pickup(this);

	PrimaryFirearm = InFirearm;

	OnFirearmPickupDelegate.Broadcast(InFirearm); 
}

void UInventoryComponent::DropFirearm_Internal(AFirearm* InFirearm)
{
	if (!InFirearm || InFirearm->IsFiring() || InFirearm->IsReloading())
	{
		return;
	}

	InFirearm->Drop();

	PrimaryFirearm = nullptr;

	OnFirearmDropDelegate.Broadcast(InFirearm);
}

void UInventoryComponent::OnRep_PrimaryFirearm()
{
	if (PrimaryFirearm)
	{
		SimulatedPrimaryFirearm = PrimaryFirearm;
		PickupFirearm_Internal(PrimaryFirearm);
	}
	else
	{
		DropFirearm_Internal(SimulatedPrimaryFirearm);
		SimulatedPrimaryFirearm = nullptr;
	}
}

AFirearm* UInventoryComponent::GetPrimaryFirearm()
{
	return PrimaryFirearm;
}

void UInventoryComponent::ServerPickupFirearm_Implementation(AFirearm* InFirearm)
{
	PickupFirearm(InFirearm);
}

void UInventoryComponent::PickupFirearm(AFirearm* InFirearm)
{
	if (GetOwnerRole() < ENetRole::ROLE_Authority)
	{
		ServerPickupFirearm(InFirearm);	
	}

	PickupFirearm_Internal(InFirearm);
}

void UInventoryComponent::DropFirearm(AFirearm* InFirearm)
{
	if (GetOwnerRole() < ENetRole::ROLE_Authority)
	{
		ServerDropFirearm(InFirearm);
	}

	DropFirearm_Internal(InFirearm);
}

void UInventoryComponent::ServerDropFirearm_Implementation(AFirearm* InFirearm)
{
	DropFirearm(InFirearm);
}
