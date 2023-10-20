#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Perception/AISightTargetInterface.h"

#include "ItemBase.generated.h"

class ACombatCharacter;
class USphereComponent;
class UAIPerceptionStimuliSourceComponent;
class UInteractableComponent;

UCLASS()
class REBELWARS_API AItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemBase();

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventory();

	virtual void OnPickup(class UInventoryComponent* InInventory);
	virtual void OnDrop();

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

	UFUNCTION()
	virtual void OnInteract(AActor* Initiator) {};

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY()
	UAIPerceptionStimuliSourceComponent* AIPerceptionStimuliSource;

	// OwningCharacter that caches during item pickup. Useful since do not require cast
	ACombatCharacter* CachedOwner;

private:
	UPROPERTY(Replicated)
	UInventoryComponent* Inventory;

	// Add ability to interact with the item, specifically - pick up
	UInteractableComponent* InteractableComponent;
};
