#include "Components/InventoryComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Firearm.h"

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	GiveStartingWeapons();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Inventory);
	DOREPLIFETIME(UInventoryComponent, EquippedFirearm);
}

void UInventoryComponent::GiveStartingWeapons()
{
	if (GetOwnerRole() < ENetRole::ROLE_Authority)
	{
		return;
	}

	for (const TSubclassOf<AFirearm>& WeaponClass : StartingWeapons)
	{
		bool bHasWeapon = false;
		for (const AFirearm* Weapon : Inventory)
		{
			if (Weapon && Weapon->IsA(WeaponClass))
			{
				bHasWeapon = true;
			}
		}

		if (!bHasWeapon)
		{
			if (AFirearm* SpawnedWeapon = GetWorld()->SpawnActor<AFirearm>(WeaponClass))
			{
				PickupFirearm(SpawnedWeapon, false);
			}
		}
	}
}

void UInventoryComponent::OnRep_Inventory()
{
	for (AFirearm* Firearm : SimulatedInventory)
	{
		if (Inventory.Find(Firearm) == INDEX_NONE)
		{
			DropFirearm(Firearm, false, true);
		}
	}

	for (AFirearm* Firearm : Inventory)
	{
		if (SimulatedInventory.Find(Firearm) == INDEX_NONE)
		{
			PickupFirearm(Firearm, true);
		}
	}

	SimulatedInventory = Inventory;
}

void UInventoryComponent::OnRep_EquippedFirearm()
{
	if (EquippedFirearm != SimulatedEquippedFirearm)
	{
		if (SimulatedEquippedFirearm)
		{
			OnFirearmUnequipDelegate.Broadcast(SimulatedEquippedFirearm);
		}

		if (EquippedFirearm)
		{
			OnFirearmEquipDelegate.Broadcast(EquippedFirearm);
		}
	}

	SimulatedEquippedFirearm = EquippedFirearm;
}

void UInventoryComponent::ServerPickupFirearm_Implementation(AFirearm* InFirearm)
{
	PickupFirearm(InFirearm);
}

void UInventoryComponent::ServerDropFirearm_Implementation(AFirearm* InFirearm, bool bAutoEquip)
{
	DropFirearm(InFirearm, bAutoEquip);
}

void UInventoryComponent::ServerEquipFirearm_Implementation(EInventorySlot InSlot)
{
	EquipFirearm(InSlot);
}

void UInventoryComponent::PickupFirearm(AFirearm* InFirearm, bool bFromReplication)
{
	if (!InFirearm || InFirearm->IsPendingKill())
	{
		return;
	}

	if (!bFromReplication)
	{
		if (GetOwnerRole() < ENetRole::ROLE_Authority)
		{
			ServerPickupFirearm(InFirearm);
			return;
		}
		else if (GetOwnerRole() == ENetRole::ROLE_Authority)
		{
			bool bShouldEquip = false;

			if (AFirearm* ReplacedWeapon = GetFirearm(InFirearm->Slot))
			{
				if (GetEquippedFirearm() == ReplacedWeapon)
				{
					if (ReplacedWeapon->IsDeploying() || ReplacedWeapon->IsFiring() || ReplacedWeapon->IsReloading())
					{
						return;
					}

					ReplacedWeapon->Unequip();
					bShouldEquip = true; // We are replacing the currently equipped weapon, so we must equip the new
				}

				DropFirearm(ReplacedWeapon, false);
			}
			else
			{
				bShouldEquip = true; // autoequip new weapons
			}
			
			Inventory.Add(InFirearm);

			if (bShouldEquip)
			{
				EquipFirearm(InFirearm->Slot);
			}
		}
	}

	InFirearm->OnPickup(this);
	OnFirearmPickupDelegate.Broadcast(InFirearm);
}

void UInventoryComponent::DropFirearm(AFirearm* InFirearm, bool bAutoEquip, bool bFromReplication)
{
	if (!InFirearm)
	{
		return;
	}

	if (!bFromReplication)
	{
		if (GetOwnerRole() < ENetRole::ROLE_Authority)
		{
			ServerDropFirearm(InFirearm, bAutoEquip);
			return;
		}
		else if (GetOwnerRole() == ENetRole::ROLE_Authority)
		{
			Inventory.Remove(InFirearm);

			if (bAutoEquip)
			{
				if (Inventory.Num())
				{
					EquipFirearm(Inventory[0]->Slot);
				}
			}

			if (!Inventory.Num())
			{
				HolsterWeapon();
			}
		}
	}
	
	InFirearm->OnDrop();
	OnFirearmDropDelegate.Broadcast(InFirearm);
}

void UInventoryComponent::DropAll()
{
	while (Inventory.Num())
	{
		DropFirearm(Inventory[0], false);
	}
}

AFirearm* UInventoryComponent::GetFirearm(EInventorySlot Slot) const
{
	for (AFirearm* Firearm : Inventory)
	{
		if (Firearm && Firearm->Slot == Slot)
		{
			return Firearm;
		}
	}

	return nullptr;
}

AFirearm* UInventoryComponent::GetEquippedFirearm() const
{
	return EquippedFirearm;
}

void UInventoryComponent::EquipFirearm(EInventorySlot InSlot, bool bFromReplication)
{
	AFirearm* PendingFirearm = GetFirearm(InSlot);
	if (!PendingFirearm)
	{
		return;
	}

	if (!bFromReplication)
	{
		if (GetOwnerRole() < ENetRole::ROLE_Authority)
		{
			ServerEquipFirearm(InSlot);
			return;
		}
		else if (GetOwnerRole() == ENetRole::ROLE_Authority)
		{
			if (EquippedFirearm)
			{
				if (PendingFirearm == EquippedFirearm || EquippedFirearm->IsFiring() || EquippedFirearm->IsDeploying() || EquippedFirearm->IsReloading())
				{
					return;
				}

				EquippedFirearm->Unequip();
				OnFirearmUnequipDelegate.Broadcast(EquippedFirearm);
			}

			EquippedFirearm = PendingFirearm;
			PendingFirearm->Equip();
			OnFirearmEquipDelegate.Broadcast(PendingFirearm);
		}
	}
}

void UInventoryComponent::HolsterWeapon()
{
	if (!EquippedFirearm)
	{
		return;
	}

	if (GetOwnerRole() == ENetRole::ROLE_Authority)
	{
		if (EquippedFirearm)
		{
			EquippedFirearm->Unequip();
			OnFirearmUnequipDelegate.Broadcast(EquippedFirearm);
			EquippedFirearm = nullptr;
		}
	}
}

const TArray<AFirearm*>& UInventoryComponent::GetWeapons() const
{
	return Inventory;
}
