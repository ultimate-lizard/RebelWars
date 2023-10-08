#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmPickup, class AFirearm*, InFirearm);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmDrop, class AFirearm*, InFirearm);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REBELWARS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	class AFirearm* GetPrimaryFirearm();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void PickupFirearm(AFirearm* InFirearm, bool bFromReplication = false);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropFirearm(AFirearm* InFirearm, bool bFromReplication = false);

public:
	UPROPERTY(BlueprintAssignable)
	FOnFirearmPickup OnFirearmPickupDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnFirearmPickup OnFirearmDropDelegate;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_PrimaryFirearm();

	UFUNCTION(Server, Reliable)
	void ServerPickupFirearm(class AFirearm* InFirearm);
	void ServerPickupFirearm_Implementation(class AFirearm* InFirearm);

	UFUNCTION(Server, Reliable)
	void ServerDropFirearm(class AFirearm* InFirearm);
	void ServerDropFirearm_Implementation(class AFirearm* InFirearm);

private:
	UPROPERTY(ReplicatedUsing=OnRep_PrimaryFirearm)
	class AFirearm* PrimaryFirearm;
	// This mirroring variable is used solely for OnRep function to remember the weapon for replicated dropping logic
	class AFirearm* SimulatedPrimaryFirearm;
};
