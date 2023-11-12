#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "InventoryComponent.generated.h"

class AFirearm;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmEquip, class AFirearm*, InFirearm);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmUnequip, class AFirearm*, InFirearm);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmPickup, class AFirearm*, InFirearm);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmDrop, class AFirearm*, InFirearm);

UENUM(BlueprintType)
enum class EInventorySlot : uint8
{
	Primary,
	Sidearm,
	Grenade,
	Melee
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REBELWARS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void PickupFirearm(AFirearm* InFirearm, bool bFromReplication = false);

	UFUNCTION(BlueprintCallable)
	void DropFirearm(AFirearm* InFirearm, bool bAutoEquip = true, bool bFromReplication = false);

	UFUNCTION(BlueprintCallable)
	void DropAll();

	UFUNCTION(BlueprintPure)
	AFirearm* GetFirearm(EInventorySlot Slot) const;

	UFUNCTION(BlueprintPure)
	AFirearm* GetEquippedFirearm() const;

	UFUNCTION(BlueprintCallable)
	void EquipFirearm(EInventorySlot InSlot, bool bFromReplication = false);

	UFUNCTION(BlueprintCallable)
	void HolsterWeapon();

	const TArray<AFirearm*>& GetWeapons() const;

	void GiveStartingWeapons();

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<AFirearm>> StartingWeapons;

	UPROPERTY(BlueprintAssignable)
	FOnFirearmEquip OnFirearmEquipDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnFirearmUnequip OnFirearmUnequipDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnFirearmPickup OnFirearmPickupDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnFirearmDrop OnFirearmDropDelegate;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Inventory();

	UFUNCTION()
	void OnRep_EquippedFirearm();

	UFUNCTION(Server, Reliable)
	void ServerPickupFirearm(AFirearm* InFirearm);
	void ServerPickupFirearm_Implementation(AFirearm* InFirearm);

	UFUNCTION(Server, Reliable)
	void ServerDropFirearm(AFirearm* InFirearm, bool bAutoEquip = true);
	void ServerDropFirearm_Implementation(AFirearm* InFirearm, bool bAutoEquip = true);

	UFUNCTION(Server, Reliable)
	void ServerEquipFirearm(EInventorySlot InSlot);
	void ServerEquipFirearm_Implementation(EInventorySlot InSlot);

	UPROPERTY(ReplicatedUsing=OnRep_Inventory)
	TArray<AFirearm*> Inventory;

	TArray<AFirearm*> SimulatedInventory;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedFirearm)
	AFirearm* EquippedFirearm;

	AFirearm* SimulatedEquippedFirearm;
};
