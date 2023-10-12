#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Utils/TeamStatics.h"
#include "GenericTeamAgentInterface.h"

#include "CombatCharacter.generated.h"

class AItemBase;
class UInteractableComponent;

UENUM(BlueprintType)
enum class ECharacterMovementType : uint8
{
	Walk,
	Run,
	Sprint,
	Crouch
};

UCLASS()
class REBELWARS_API ACombatCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ACombatCharacter();

	UFUNCTION(Exec)
	void DebugDropWeapon();

	UFUNCTION(BlueprintPure)
	bool IsArmed() const;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Movement")
	void SetMovementType(ECharacterMovementType InMovementType);
	void SetMovementType_Implementation(ECharacterMovementType InMovementType);

	UFUNCTION(BlueprintPure, Category = "Movement")
	ECharacterMovementType GetMovementType() const;

	float GetCurrentHealth() const;

	virtual void AttachWeaponMesh(class AFirearm* InFirearm);
	USkeletalMeshComponent* GetHandsMesh1P();
	USkeletalMeshComponent* GetWeaponMesh1P();
	UStaticMeshComponent* GetWeaponMesh3P();

	void StartPrimaryFire();
	void StopPrimaryFire();

	// INPUT
	void MoveForward(float InRate);
	void MoveRight(float InRate);
	void LookUp(float InRate);
	void Turn(float InRate);
	void Reload();
	void DropFirearm();
	void Use();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ViewModelOffset = FVector(-10.0f, 5.0f, -15.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = "0.0"))
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = "0.0"))
	float MaxWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = "0.0"))
	float MaxRunSpeed;

	UPROPERTY(EditAnywhere, Category = "Team")
	EAffiliation Affiliation;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	void UpdateViewModelTransform();
	void UpdateBodyRotation(float DeltaTime);
	void TraceInteractables();

	virtual void Kill();
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	void OnHealthUpdate();

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void FirearmPickup(class AFirearm* InFirearm);

	UFUNCTION()
	void FirearmDrop(class AFirearm* InFirearm);

	UFUNCTION(Server, Reliable)
	void ServerPrimaryFire(AFirearm* InFirearm);
	void ServerPrimaryFire_Implementation(AFirearm* InFirearm);

	UFUNCTION(Server, Reliable)
	void ServerUse();
	void ServerUse_Implementation();

	UFUNCTION(NetMulticast, Reliable, Category = "Movement")
	void BroadcastUpdateMovement();
	void BroadcastUpdateMovement_Implementation();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	class USkeletalMeshComponent* HandsMesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	class USkeletalMeshComponent* WeaponMesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	class UStaticMeshComponent* WeaponMesh3P;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth)
	float CurrentHealth;

	UPROPERTY(Replicated)
	ECharacterMovementType MovementType;

	FGenericTeamId TeamId;

	UPROPERTY(Transient, BlueprintReadOnly)
	UInteractableComponent* FocusedInteractable;

	FRotator TargetRotation;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	FRotator HeadRotation;
};
