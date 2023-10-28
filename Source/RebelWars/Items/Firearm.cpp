#include "Items/Firearm.h"

#include "Components/SphereComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/InventoryComponent.h"
#include "Characters/CombatCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Controllers/HumanPlayerController.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

AFirearm::AFirearm()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GripType = EFirearmGripType::Rifle;
	CurrentFireMode = EFirearmFireMode::SemiAuto;
	bHasSlideLock = false;

	FireRate = 600;
	ReloadLength = 2.0f;
	ReloadDryLength = 3.0f;
	DeployLength = 0.5f;
	Damage = 20.0f;
	BulletsPerShot = 1;
	SpreadAngleRange = FVector2D(0.0f, 15.0f);
	ViewPunch = 50.0f;
	BurstResetTime = 1.0f;
	MovementSpreadPenalty = 15.0f;

	MagAmmoCapacity = 30;
	ReserveAmmoCapacity = 120;

	TimeSinceLastShot = 0.0f;

	FirearmState = EFirearmState::Idle;
	bIsDeployed = false;

	bLastShotDry = false;

	ShotsInCurrentBurst = 0;
}

void AFirearm::PlayPrimaryShotEffects()
{
	if (!CachedOwner)
	{
		return;
	}
	
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, CachedOwner->GetActorLocation());

	CachedOwner->PlayAnimMontage(FirearmAnimations.PrimaryFire3P);

	// Dry fire montage
	if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
	{
		if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
		{
			UAnimMontage* FireMontage = FirearmAnimations.PrimaryFire;

			if (CurrentMagAmmo <= 0)
			{
				if (FirearmAnimations.PrimaryFireDry)
				{
					FireMontage = FirearmAnimations.PrimaryFireDry;
				}
			}

			WeaponAnimInstance->Montage_Play(FireMontage);
		}
	}

	static const FName MuzzleSocketName = TEXT("muzzle");

	if (USkeletalMeshComponent* CharacterMesh = CachedOwner->GetMesh())
	{
		if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
		{
			if (CachedOwner->IsLocallyControlled() && CachedOwner->IsPlayerControlled())
			{
				UGameplayStatics::SpawnEmitterAttached(MuzzleFlashParticle, WeaponMesh1P, MuzzleSocketName);
			}
			else
			{
				UGameplayStatics::SpawnEmitterAttached(MuzzleFlashParticle, CharacterMesh, MuzzleSocketName);
			}
		}
	}
}

float AFirearm::GetTimePerShot() const
{
	const float ShotsPerSecond = (float)FireRate / 60.0f;
	return 1.0 / ShotsPerSecond;
}

void AFirearm::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFirearm, CurrentMagAmmo);
	DOREPLIFETIME(AFirearm, CurrentReserveAmmo);
	DOREPLIFETIME(AFirearm, FirearmState);
	DOREPLIFETIME(AFirearm, bIsDeployed);
}

void AFirearm::BeginPlay()
{
	Super::BeginPlay();

	CurrentMagAmmo = MagAmmoCapacity;
	CurrentReserveAmmo = ReserveAmmoCapacity;
}

void AFirearm::Tick(float DeltaTime)
{
	switch (CurrentFireMode)
	{
	case EFirearmFireMode::Auto:
		if (FirearmState == EFirearmState::Firing && IsReadyForNextShot() && CanFire())
		{
			PrimaryFire();
		}
		break;
	}

	if (ShotsInCurrentBurst && GetWorld()->GetTimeSeconds() - TimeSinceLastShot >= BurstResetTime)
	{
		ShotsInCurrentBurst = 0;
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Burst has been reset"));
	}
}

void AFirearm::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// GetInteractableComponent()->OnInteract.AddDynamic(this, &AFirearm::OnInteract);
}

void AFirearm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ReloadTimer.Invalidate();
	DeployTimer.Invalidate();
}

void AFirearm::OnInteract(AActor* Initiator)
{
	if (CachedOwner)
	{
		return;
	}

	Super::OnInteract(Initiator);

	if (UInventoryComponent* InitiatorInventory = Initiator->FindComponentByClass<UInventoryComponent>())
	{
		InitiatorInventory->PickupFirearm(this);
	}
}

void AFirearm::Equip()
{
	GetWorldTimerManager().ClearTimer(DeployTimer);
	GetWorldTimerManager().SetTimer(DeployTimer, this, &AFirearm::OnFinishDeploy, DeployLength);
}

void AFirearm::Unequip()
{
	bIsDeployed = false;
	GetWorldTimerManager().ClearTimer(DeployTimer);
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	FirearmState = EFirearmState::Idle;
}

void AFirearm::StartPrimaryFire()
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerStartPrimaryFire();
		return;
	}

	if (CanFire())
	{
		switch (CurrentFireMode)
		{
		case EFirearmFireMode::Auto:
			FirearmState = EFirearmState::Firing;
			break;
		case EFirearmFireMode::SemiAuto:
			if (IsReadyForNextShot())
			{
				FirearmState = EFirearmState::Firing;
				PrimaryFire();
			}
			break;
		}
	}
}

void AFirearm::StopPrimaryFire()
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerStopPrimaryFire();
		return;
	}

	if (IsFiring())
	{
		FirearmState = EFirearmState::Idle;
	}
}

void AFirearm::ServerStartPrimaryFire_Implementation()
{
	StartPrimaryFire();
}

void AFirearm::ServerStopPrimaryFire_Implementation()
{
	StopPrimaryFire();
}

void AFirearm::PrimaryFire()
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		if (CurrentMagAmmo > 0)
		{
			--CurrentMagAmmo;
		}
	}

	if (UWorld* World = GetWorld())
	{
		TimeSinceLastShot = World->GetTimeSeconds();
	}

	TraceBullet();

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (AHumanPlayerController* PlayerController = Cast<AHumanPlayerController>(OwnerCharacter->GetController()))
		{
			PlayerController->AddViewPunch(FRotator::MakeFromEuler(FVector(0.0f, -ViewPunch, 0.0f)));
		}
	}

	PlayPrimaryShotEffects();

	ShotsInCurrentBurst++;

	FString ShotsCountStr = FString::Printf(TEXT("Burst: %i"), ShotsInCurrentBurst);
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple, *ShotsCountStr);
}

bool AFirearm::IsReadyForNextShot() const
{
	if (UWorld* World = GetWorld())
	{
		const float WorldTime = World->GetTimeSeconds();
		bool bHasEnoughTimePassed = WorldTime >= TimeSinceLastShot + GetTimePerShot();

		return bHasEnoughTimePassed;
	}

	return false;
}

void AFirearm::Reload(bool bFromReplication)
{
	if (!bFromReplication)
	{
		if (GetLocalRole() < ENetRole::ROLE_Authority)
		{
			ServerReload();
			return;
		}
	}

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		if (!CanReload())
		{
			return;
		}

		if (GetLocalRole() == ENetRole::ROLE_Authority)
		{
			bLastShotDry = CurrentMagAmmo <= 0;

			FirearmState = EFirearmState::Reloading;

			GetWorldTimerManager().SetTimer(ReloadTimer, this, &ThisClass::OnFinishReload, CurrentMagAmmo > 0 ? ReloadLength : ReloadDryLength);
		}
	}

	// Animations
	if (!CachedOwner)
	{
		return;
	}

	// Dry reload
	if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
	{
		if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
		{
			UAnimMontage* ReloadMontage = FirearmAnimations.Reload;

			if (CurrentMagAmmo <= 0)
			{
				if (FirearmAnimations.ReloadDry)
				{
					ReloadMontage = FirearmAnimations.ReloadDry;
				}
			}

			WeaponAnimInstance->Montage_Play(ReloadMontage);
		}
	}

	// Reload 3P
	if (CachedOwner)
	{
		CachedOwner->PlayAnimMontage(FirearmAnimations.Reload3P);
	}
}

void AFirearm::OnFinishReload()
{
	int32 TotalAmmo = CurrentMagAmmo + CurrentReserveAmmo;
	if (TotalAmmo - MagAmmoCapacity >= 0)
	{
		TotalAmmo -= MagAmmoCapacity;
		CurrentMagAmmo = MagAmmoCapacity;
		CurrentReserveAmmo = TotalAmmo;
	}
	else
	{
		CurrentMagAmmo = TotalAmmo;
		CurrentReserveAmmo = 0;
	}

	FirearmState = EFirearmState::Idle;

	GetWorldTimerManager().ClearTimer(ReloadTimer);
}

void AFirearm::TraceBullet()
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		if (UWorld* World = GetWorld())
		{
			if (ACombatCharacter* OwnerPawn = Cast<ACombatCharacter>(GetOwner()))
			{
				FVector EyesLocation;
				FRotator EyesRotation;
				OwnerPawn->GetActorEyesViewPoint(EyesLocation, EyesRotation);

				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
				QueryParams.AddIgnoredActor(CachedOwner);

				TMap<AActor*, TArray<FHitResult>> UniqueActorsHits;

				float SpreadAngle = SpreadAngleRange.X;

				if (SpreadStrengthCurve)
				{
					float RecoilModifier = SpreadStrengthCurve->GetFloatValue(static_cast<float>(ShotsInCurrentBurst));
					FString RecoilStr = FString::Printf(TEXT("Recoil: %f"), RecoilModifier);
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, *RecoilStr);
					const FVector2D CurveRange(0.0f, 1.0f);
					SpreadAngle = FMath::GetMappedRangeValueUnclamped(CurveRange, SpreadAngleRange, RecoilModifier);
				}

				// Movement penalt
				if (CachedOwner && CachedOwner->GetVelocity().Size())
				{
					const FVector2D VelocityRange(0.0f, CachedOwner->MaxRunSpeed);
					const FVector2D VelocityModifierDegreesRange(0.0f, MovementSpreadPenalty);
					const float Penalty = FMath::GetMappedRangeValueUnclamped(VelocityRange, VelocityModifierDegreesRange, CachedOwner->GetVelocity().Size());
					FString PenaltyStr = FString::Printf(TEXT("Penalty: %f"), Penalty);
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, *PenaltyStr);
					SpreadAngle += Penalty;
				}

				for (int32 i = 0; i < BulletsPerShot; ++i)
				{
					// TODO: Max bullet distance
					static const float MaxTraceDistance = 1'000'000.0f;

					FVector RandDir = FMath::VRandCone(UKismetMathLibrary::GetForwardVector(EyesRotation), FMath::DegreesToRadians(SpreadAngle));

					TArray<FHitResult> Hits;
					FVector TraceDestination = EyesLocation + RandDir * MaxTraceDistance;
					World->LineTraceMultiByChannel(Hits, EyesLocation, TraceDestination, ECollisionChannel::ECC_Visibility, QueryParams);

					for (FHitResult& Hit : Hits)
					{
						DrawDebugPoint(GetWorld(), Hit.Location, 10.0f, FColor::Red, true);

						if (AActor* HitActor = Hit.Actor.Get())
						{
							if (HitActor->CanBeDamaged())
							{
								if (CachedOwner)
								{
									FVector Direction = Hit.TraceEnd - Hit.TraceStart;
									Direction.Normalize();

									UGameplayStatics::ApplyPointDamage(HitActor, Damage, Direction, Hit, CachedOwner->GetController(), this, UDamageType::StaticClass());
								}
							}
						}
					}
				}
			}
		}
	}
}

void AFirearm::BroadcastDebugEffects_Implementation(FVector Location)
{
	if (UWorld* World = GetWorld())
	{
		DrawDebugPoint(World, Location, 20.0f, FColor::Red, true);
	}
}

bool AFirearm::CanFire() const
{
	return CurrentMagAmmo > 0 && !IsReloading() && IsDeployed();
}

bool AFirearm::IsFiring() const
{
	return FirearmState == EFirearmState::Firing || !IsReadyForNextShot();
}

void AFirearm::ServerReload_Implementation()
{
	Reload();
}

void AFirearm::OnRep_FirearmState()
{
	switch (FirearmState)
	{
	case EFirearmState::Reloading:
		Reload(true);
		break;
	case EFirearmState::Firing:
		if (CurrentFireMode == EFirearmFireMode::SemiAuto)
		{
			PrimaryFire();
		}
		break;
	}
}

bool AFirearm::CanReload() const
{
	return !IsFiring() && !IsReloading() && CurrentMagAmmo < MagAmmoCapacity && CurrentReserveAmmo > 0 && IsDeployed();
}

bool AFirearm::IsReloading() const
{
	return FirearmState == EFirearmState::Reloading;
}

bool AFirearm::IsDeployed() const
{
	return bIsDeployed;
}

bool AFirearm::IsDeploying() const
{
	return !IsDeployed() && GetWorldTimerManager().IsTimerActive(DeployTimer);
}

//float AFirearm::GetShotsPerSecond() const
//{
//	float ShotsPerSecond = 1 / (FPlatformTime::Seconds() - LastShotSeconds);
//
//	FString ShotsPerSecondStr = FString::Printf(TEXT("%f"), ShotsPerSecond);
//	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple, *ShotsPerSecondStr);
//
//	return ShotsPerSecond;
//}

void AFirearm::OnFinishDeploy()
{
	bIsDeployed = true;
	GetWorldTimerManager().ClearTimer(DeployTimer);
}
