#pragma once

#include "CoreMinimal.h"
#include "Items/ItemBase.h"
#include "Sound/SoundCue.h"
#include "Components/InventoryComponent.h"

#include "Firearm.generated.h"

UENUM(BlueprintType)
enum class EFirearmState : uint8
{
	Idle,
	Firing,
	Reloading
};

UENUM(BlueprintType)
enum class EFirearmGripType : uint8
{
	Rifle,
	Pistol,
	Shoulder,
};

UENUM(BlueprintType)
enum class EFirearmFireMode : uint8
{
	SemiAuto,
	Auto
};

USTRUCT(BlueprintType)
struct FFirearmAnimations
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* PrimaryFire3P;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Reload3P;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* PrimaryFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* PrimaryFireDry;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Deploy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* ReloadDry;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* ReloadStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* ReloadEnd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* ReloadEndDry;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* LoadRound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* Default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimSequence* DefaultDry;
};

/* A simple automatic or semi-automatic weapon */
UCLASS()
class REBELWARS_API AFirearm : public AItemBase
{
	GENERATED_BODY()
	
public:
	AFirearm();

	virtual void Equip();
	virtual void Unequip();

	virtual void StartPrimaryFire();
	virtual void StopPrimaryFire();
	virtual bool CanFire() const;
	bool IsFiring() const;

	virtual void Reload(bool bFromReplication = false);
	virtual bool CanReload() const;
	bool IsReloading() const;

	bool IsDeployed() const;
	bool IsDeploying() const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EFirearmGripType GripType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EInventorySlot Slot;

	// TODO: Implement proper switching
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFirearmFireMode CurrentFireMode;

	// Should animation consider slide lock
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHasSlideLock;

	// Rate of fire of the firearm in shots per minute
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0"))
	int32 FireRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0"))
	float ReloadStartLength;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0"))
	float ReloadEndLength;

	// How much time does the tactical reload take in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0"))
	float ReloadLength;

	// How much time does the dry reload take in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0"))
	float ReloadDryLength;

	// How much time does it take to deploy the weapon
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.1"))
	float DeployLength;

	// How many bullets comes out from the gun during one shot. Useful for shotguns
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Damage;

	// How many bullets comes out from the gun during one shot. Useful for shotguns
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0"))
	int32 BulletsPerShot;

	/* How much the bullets spread in a cone half angle(in degrees).
		X: minimum spread angle. Y: maximum spread angle */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0"), Category = "Recoil")
	FVector2D SpreadAngleRange;

	/* Time: shots made in a burst. Value: the strength of a spread angle from 0.0f to 1.0f.
		If no curve specified, the strength of the spread is the minimum value of SpreadAngleRange */ 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	UCurveFloat* SpreadStrengthCurve;

	// How much in Y degrees the view is punched per shot. Does not affect spread and control rotation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float ViewPunch;

	// Time in seconds before the amount of shots in the current burst resets to 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float BurstResetTime;

	// The maximum angle deviation (in degrees) that movement causes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float MovementSpreadPenalty;

	// How many shells fit into the magazine
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", Meta = (ClampMin = "0"))
	int32 MagAmmoCapacity;

	// How many shells can the character carry excluding the MagCapacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", Meta = (ClampMin = "0"))
	int32 ReserveAmmoCapacity;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Replicated, Transient, Category = "Ammo", Meta = (ClampMin = "0"))
	int32 CurrentMagAmmo;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Replicated, Transient, Category = "Ammo", Meta = (ClampMin = "0"))
	int32 CurrentReserveAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FFirearmAnimations FirearmAnimations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* MuzzleFlashParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundCue* FireSound;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnInteract(AActor* Initiator) override;

	virtual void PrimaryFire();

	virtual void OnFinishReload();
	virtual void OnFinishDeploy();

	virtual void PlayPrimaryShotEffects();

	float GetTimePerShot() const;
	bool IsReadyForNextShot() const;

	UFUNCTION()
	void OnRep_FirearmState();

	UFUNCTION(Server, Reliable)
	void ServerStartPrimaryFire();
	void ServerStartPrimaryFire_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerStopPrimaryFire();
	void ServerStopPrimaryFire_Implementation();

	UFUNCTION(Server, Reliable)
	void ServerReload();
	void ServerReload_Implementation();

	// TODO: Implement bullet system?
	virtual void TraceBullet();

	UFUNCTION(NetMulticast, Unreliable)
	void BroadcastDebugEffects(FVector Location);
	void BroadcastDebugEffects_Implementation(FVector Location);

protected:
	FTimerHandle ReloadTimer;
	FTimerHandle DeployTimer;

	float TimeSinceLastShot;

	UPROPERTY(ReplicatedUsing=OnRep_FirearmState)
	EFirearmState FirearmState;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDeployed;

	bool bLastShotDry;

	// Resets to 0 after BurstResetTime passes
	uint32 ShotsInCurrentBurst;
};
