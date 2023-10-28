#include "Items/FirearmSequential.h"

#include "Characters/CombatCharacter.h"

void AFirearmSequential::Reload(bool bFromReplication)
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

		bLastShotDry = CurrentMagAmmo <= 0;
	}

	StartSequentialReload();
}

void AFirearmSequential::StartSequentialReload()
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		FirearmState = EFirearmState::Reloading;
	}	

	// Play start anim
	if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
	{
		if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
		{
			WeaponAnimInstance->Montage_Play(FirearmAnimations.ReloadStart);
		}
	}

	GetWorldTimerManager().SetTimer(ReloadTimer, this, &ThisClass::LoadRound, ReloadStartLength);
}

void AFirearmSequential::EndSequentialReload()
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		FirearmState = EFirearmState::Idle;
	}

	if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
	{
		if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
		{
			WeaponAnimInstance->Montage_Play(bLastShotDry ? FirearmAnimations.ReloadEndDry : FirearmAnimations.ReloadEnd);
		}
	}
}

void AFirearmSequential::LoadRound()
{
	if (CurrentMagAmmo < MagAmmoCapacity && CurrentReserveAmmo > 0)
	{
		GetWorldTimerManager().SetTimer(ReloadTimer, this, &ThisClass::OnRoundLoaded, ReloadLength);

		if (USkeletalMeshComponent* WeaponMesh1P = CachedOwner->GetWeaponMesh1P())
		{
			if (UAnimInstance* WeaponAnimInstance = WeaponMesh1P->GetAnimInstance())
			{
				WeaponAnimInstance->Montage_Play(FirearmAnimations.LoadRound);
			}
		}

		if (CachedOwner)
		{
			CachedOwner->PlayAnimMontage(FirearmAnimations.Reload3P);
		}
	}
	else
	{
		EndSequentialReload();
	}
}

void AFirearmSequential::OnRoundLoaded()
{
	--CurrentReserveAmmo;
	++CurrentMagAmmo;

	GetWorldTimerManager().ClearTimer(ReloadTimer);

	LoadRound();
}
