#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Perception/AISightTargetInterface.h"

#include "ItemBase.generated.h"

class ACombatCharacter;
class USphereComponent;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class REBELWARS_API AItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemBase();

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventory();

	virtual void Pickup(class UInventoryComponent* InInventory);
	virtual void Drop();

	UStaticMeshComponent* GetPickupMesh();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	UStaticMesh* Mesh3P;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	USkeletalMesh* Mesh1P;

protected:
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void DropOnGround();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(Replicated)
	UInventoryComponent* Inventory;

	UPROPERTY()
	UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSource;

	// OwningCharacter that caches during item pickup. Useful since do not require cast
	ACombatCharacter* CachedOwner;
};
