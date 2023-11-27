#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Utils/TeamStatics.h"
#include "GenericTeamAgentInterface.h"

#include "CombatCharacter.generated.h"

class AItemBase;
class UInteractableComponent;
class ACombatCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKill, AActor*, Killer, ACombatCharacter*, Victim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResurrect, ACombatCharacter*, ResurrectedActor);

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

	virtual void SetPlayerDefaults() override;
	virtual void Landed(const FHitResult& Hit) override;

	UFUNCTION(BlueprintPure)
	bool IsArmed() const;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Movement")
	void SetMovementType(ECharacterMovementType InMovementType);
	void SetMovementType_Implementation(ECharacterMovementType InMovementType);

	UFUNCTION(BlueprintPure, Category = "Movement")
	ECharacterMovementType GetMovementType() const;

	void SetHealth(float NewHealth);
	UFUNCTION(BlueprintPure)
	float GetCurrentHealth() const;

	UFUNCTION(NetMulticast, Reliable)
	void BroadcastBecomeRagdoll(FVector ImpulseDirection = FVector::ZeroVector, FVector ImpulseLocation = FVector::ZeroVector);
	void BroadcastBecomeRagdoll_Implementation(FVector ImpulseDirection = FVector::ZeroVector, FVector ImpulseLocation = FVector::ZeroVector);

	UFUNCTION(BlueprintPure)
	bool IsDead() const;

	virtual void AttachWeaponMesh(class AFirearm* InFirearm);
	USkeletalMeshComponent* GetHandsMesh1P();
	USkeletalMeshComponent* GetWeaponMesh1P();
	UStaticMeshComponent* GetWeaponMesh3P();

	void StartPrimaryFire();
	void StopPrimaryFire();

	// Input
	void MoveForward(float InRate);
	void MoveRight(float InRate);
	void LookUp(float InRate);
	void Turn(float InRate);
	void Reload();
	void DropFirearm();
	void Use();

	template<int32 Index>
	inline void SelectWeaponSlot()
	{
		SelectWeaponSlot(Index);
	}

	void SelectWeaponSlot(int32 InIndex);

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

	UPROPERTY(BlueprintAssignable)
	FOnKill OnKillDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnResurrect OnResurrectDelegate;

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

	void TickViewModelTransform(float DeltaTime);
	void TickBodyRotation(float DeltaTime);
	void TickWeaponSway(float DeltaTime);
	void TraceInteractables();

	virtual void Kill();
	virtual void Resurrect();
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	void OnHealthUpdate();

	void SetRagdollEnabled(bool bEnableRagdoll);
	bool IsRagdoll() const;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void FirearmEquip(class AFirearm* InFirearm);

	UFUNCTION()
	void FirearmUnequip(class AFirearm* InFirearm);

	UFUNCTION(Server, Reliable)
	void ServerUse();
	void ServerUse_Implementation();

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

	bool bIsDead;

	FTimerHandle DamageAccumulationTimer;

	float AccumulatedDamage;

	UPROPERTY(Replicated)
	AActor* LastDamageCauser;

	UPROPERTY(Replicated)
	ECharacterMovementType MovementType;

	FGenericTeamId TeamId;

	UPROPERTY(Transient, BlueprintReadOnly)
	UInteractableComponent* FocusedInteractable;

	UPROPERTY(Replicated)
	FRotator TargetActorRotation;

	UPROPERTY(Replicated)
	FRotator HeadRotation;

	float ViewModelSwayCycle = -3.0f;
	float ViewModelSway = 0.0f;
};
