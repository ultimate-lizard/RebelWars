#include "Items/Firearm.h"

#include "Components/SphereComponent.h"
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

	MagAmmoCapacity = 30;
	ReserveAmmoCapacity = 120;

	TimeSinceLastShot = 0.0f;

	FirearmState = EFirearmState::Idle;
	bIsDeployed = false;
}

void AFirearm::PlayPrimaryShotEffects()
{
	if (!CachedOwner)
	{
		return;
	}
	
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, CachedOwner->GetActorLocation());

	CachedOwner->PlayAnimMontage(FirearmAnimations.PrimaryFire3P);

	if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
	{
		if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
		{
			WeaponAnimInstance->Montage_Play(CurrentMagAmmo > 0 ? FirearmAnimations.PrimaryFire : FirearmAnimations.PrimaryFireDry);
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
	if (FirearmState == EFirearmState::Firing)
	{
		if (IsReadyForNextShot() && CanFire())
		{
			PrimaryFire();
			PlayPrimaryShotEffects();
		}
	}
}

void AFirearm::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ensure(PickupTraceSphere);
	PickupTraceSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnPickupBeginOverlap);
}

void AFirearm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	PickupTraceSphere->OnComponentBeginOverlap.RemoveAll(this);
	ReloadTimer.Invalidate();
	DeployTimer.Invalidate();
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
	}

	if (CanFire())
	{
		FirearmState = EFirearmState::Firing;
	}
}

void AFirearm::StopPrimaryFire()
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerStopPrimaryFire();
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

		if (CurrentFireMode == EFirearmFireMode::SemiAuto)
		{
			FirearmState = EFirearmState::Idle;
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
			PlayerController->AddViewPunch(FRotator(FMath::RandRange(-1.0f, 2.5f), FMath::RandRange(-1.0f, 1.0f), 0.0f));
		}
	}
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

void AFirearm::Reload_Internal()
{
	if (CanReload())
	{
		FirearmState = EFirearmState::Reloading;

		GetWorldTimerManager().SetTimer(ReloadTimer, this, &ThisClass::OnFinishReload, CurrentMagAmmo > 0 ? ReloadLength : ReloadDryLength);

		// Play effects

		if (!CachedOwner)
		{
			return;
		}

		if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
		{
			if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
			{
				if (CurrentMagAmmo > 0)
				{
					WeaponAnimInstance->Montage_Play(FirearmAnimations.Reload);
				}
				else
				{
					WeaponAnimInstance->Montage_Play(FirearmAnimations.ReloadDry);
				}
			}
		}

		if (CachedOwner)
		{
			CachedOwner->PlayAnimMontage(FirearmAnimations.Reload3P);
		}
	}
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

				TArray<FHitResult> Hits;
				World->LineTraceMultiByChannel(Hits, EyesLocation, EyesLocation + EyesRotation.Vector() * 10000.0f, ECollisionChannel::ECC_Visibility, QueryParams);

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

void AFirearm::Reload()
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerReload();
	}

	Reload_Internal();
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
		Reload_Internal();
		break;
	}
}

bool AFirearm::CanReload() const
{
	return !IsReloading() && CurrentMagAmmo < MagAmmoCapacity&& CurrentReserveAmmo > 0;
}

bool AFirearm::IsReloading() const
{
	return FirearmState == EFirearmState::Reloading;
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

void AFirearm::OnFinishDeploy()
{
	bIsDeployed = true;
	GetWorldTimerManager().ClearTimer(DeployTimer);
}

void AFirearm::OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!GetOwner() && GetLocalRole() == ENetRole::ROLE_Authority)
	{
		if (auto InventoryComponent = OtherActor->FindComponentByClass<UInventoryComponent>())
		{
			if (!InventoryComponent->GetPrimaryFirearm())
			{
				InventoryComponent->PickupFirearm(this);
			}
		}
	}
}
