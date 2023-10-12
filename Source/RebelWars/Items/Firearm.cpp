#include "Items/Firearm.h"

#include "Components/SphereComponent.h"
#include "Components/InteractableComponent.h"
#include "Items/InventoryComponent.h"
#include "Characters/CombatCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Controllers/HumanPlayerController.h"
#include "DrawDebugHelpers.h"

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
	BulletsPerShot = 1;
	Spread = 0.0f;
	ViewPunchConfig.PitchSpread = FVector2D(-1.0f, 2.5f);
	ViewPunchConfig.YawSpread = FVector2D(-1.0f, 1.0f);

	MagAmmoCapacity = 30;
	ReserveAmmoCapacity = 120;

	TimeSinceLastShot = 0.0f;

	FirearmState = EFirearmState::Idle;
	bIsDeployed = false;

	bLastShotDry = false;
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
	if (CurrentFireMode == EFirearmFireMode::Auto && FirearmState == EFirearmState::Firing && IsReadyForNextShot() && CanFire())
	{
		PrimaryFire();
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

void AFirearm::Pickup(UInventoryComponent* InInventory)
{
	Super::Pickup(InInventory);

	if (!GetWorldTimerManager().IsTimerActive(DeployTimer))
	{
		GetWorldTimerManager().SetTimer(DeployTimer, this, &AFirearm::OnFinishDeploy, DeployLength);
	}
}

void AFirearm::Drop()
{
	Super::Drop();

	bIsDeployed = false;
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
			PlayerController->AddViewPunch(FRotator(FMath::RandRange(ViewPunchConfig.PitchSpread.X, ViewPunchConfig.PitchSpread.Y), FMath::RandRange(ViewPunchConfig.YawSpread.X, ViewPunchConfig.YawSpread.Y), 0.0f));
		}
	}

	PlayPrimaryShotEffects();
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

				for (int32 i = 0; i < BulletsPerShot; ++i)
				{
					float SpreadSqrt = FMath::Sqrt(Spread);

					FVector RandomizedSpread;
					RandomizedSpread.X = FMath::RandRange(-SpreadSqrt, SpreadSqrt);
					RandomizedSpread.Y = FMath::RandRange(-SpreadSqrt, SpreadSqrt);
					RandomizedSpread.Z = FMath::RandRange(-SpreadSqrt, SpreadSqrt);

					static const float MaxTraceDistance = 1'000'000.0f;
					static const float RandomizedSpreadScale = 0.01f;

					TArray<FHitResult> Hits;
					FVector TraceDestination = EyesLocation + (EyesRotation.Vector() + RandomizedSpread * RandomizedSpreadScale) * MaxTraceDistance;
					World->LineTraceMultiByChannel(Hits, EyesLocation, TraceDestination, ECollisionChannel::ECC_Visibility, QueryParams);

					for (FHitResult& Hit : Hits)
					{
						if (Hit.Actor != this)
						{
							BroadcastDebugEffects(Hit.Location);
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
	return CurrentMagAmmo > 0 && !IsReloading() && bIsDeployed;
}

bool AFirearm::IsFiring() const
{
	return FirearmState == EFirearmState::Firing;
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
	return !IsFiring() && !IsReloading() && CurrentMagAmmo < MagAmmoCapacity&& CurrentReserveAmmo > 0;
}

bool AFirearm::IsReloading() const
{
	return FirearmState == EFirearmState::Reloading;
}

void AFirearm::OnFinishDeploy()
{
	bIsDeployed = true;
	GetWorldTimerManager().ClearTimer(DeployTimer);
}
